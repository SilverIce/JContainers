#pragma once

#include <vector>
#include <string>
#include <assert.h>

#include <boost/serialization/split_member.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include "common/ITypes.h"
#include "common/IDebugLog.h"
#include "skse/GameForms.h"

//#include "tes_context.h"
#include "object/object_base.h"
#include "skse.h"

namespace collections {

	class tes_context;

    template<class T>
    class collection_base : public object_base
    {
        collection_base(const collection_base&);
        collection_base& operator=(const collection_base&);

    protected:

        //static_assert(std::is_base_of<collection_base<T>, T>::value, "");

        explicit collection_base() : object_base((CollectionType)T::TypeId) {}

    public:

        // just for convence to not use static_cast's
        object_base& base() { return *this; }
        const object_base& base() const { return *this; }

        typedef typename object_stack_ref_template<T> ref;
        typedef typename object_stack_ref_template<const T> cref;

        static T& make(tes_context& context /*= tes_context::instance()*/) {
            auto& obj = *new T();
            obj.set_context(context);
            obj._registerSelf();
            return obj;
        }

        template<class Init>
        static T& _makeWithInitializer(Init& init, tes_context& context /*= tes_context::instance()*/) {
            auto& obj = *new T();
            obj.set_context(context);
            init(obj);
            obj._registerSelf();
            return obj;
        }

        static T& object(tes_context& context /*= tes_context::instance()*/) {
            return make(context);
        }

        template<class Init>
        static T& objectWithInitializer(Init& init, tes_context& context /*= tes_context::instance()*/) {
            return _makeWithInitializer(init, context);
        }
    };

    
    template<class R, class F>
    inline R perform_on_object_and_return(object_base & container, F& func) {
        switch (container.type()) {
        case array::TypeId:
            return func(container.as_link<array>());
        case map::TypeId:
            return func(container.as_link<map>());
        case form_map::TypeId:
            return func(container.as_link<form_map>());
        case integer_map::TypeId:
            return func(container.as_link<integer_map>());
        default:
            assert(false);
            noreturn_func();
            break;
        }
    }

    template<class R, class F>
    inline R perform_on_object_and_return(const object_base & container, F& func) {
        return perform_on_object_and_return<R>(const_cast<object_base&>(container), func);
    }

    template<class F>
    inline void perform_on_object(object_base & container, F& func) {
        switch (container.type()) {
        case array::TypeId:
            func(container.as_link<array>());
            break;
        case map::TypeId:
            func(container.as_link<map>());
            break;
        case form_map::TypeId:
            func(container.as_link<form_map>());
            break;
        case integer_map::TypeId:
            func(container.as_link<integer_map>());
            break;
        default:
            assert(false);
            break;
        }
    }

    template<class F>
    inline void perform_on_object(const object_base & container, F& func) {
        perform_on_object(const_cast<object_base&>(container), func);
    }

    enum FormId : uint32_t {
        FormZero = 0,
        FormGlobalPrefix = 0xFF,
    };

    class array;
    class map;
    class object_base;
    class Item;

    enum item_type {
        no_item = 0,
        none,
        integer,
        real,
        form,
        object,
        string,
    };

    class Item
    {
    public:
        typedef boost::blank blank;
        typedef Float32 Real;
        typedef boost::variant<boost::blank, SInt32, Real, FormId, internal_object_ref, std::string> variant;

    private:
        variant _var;

    private:

        template<class T> struct type2index{ };

        template<> struct type2index < boost::blank >  { static const item_type index = none; };
        template<> struct type2index < SInt32 >  { static const item_type index = integer; };
        template<> struct type2index < Real >  { static const item_type index = real; };
        template<> struct type2index < FormId >  { static const item_type index = form; };
        template<> struct type2index < internal_object_ref >  { static const item_type index = object; };
        template<> struct type2index < std::string >  { static const item_type index = string; };

        static_assert(type2index<Real>::index > type2index<SInt32>::index, "Item::type2index works incorrectly");

    public:

        void u_nullifyObject() {
            if (auto ref = boost::get<internal_object_ref>(&_var)) {
                ref->jc_nullify();
            }
        }

        Item() {}
        Item(Item&& other) : _var(std::move(other._var)) {}
        Item(const Item& other) : _var(other._var) {}

        Item& operator = (Item&& other) {
            _var = std::move(other._var);
            return *this;
        }

        Item& operator = (const Item& other) {
            _var = other._var;
            return *this;
        }

        const variant& var() const {
            return _var;
        }

        variant& var() {
            return _var;
        }

        template<class T> bool is_type() const {
            return boost::get<T>(&_var) != nullptr;
        }

        item_type type() const {
            return item_type(_var.which() + 1);
        }

        template<class T> T* get() {
            return boost::get<T>(&_var);
        }

        template<class T> const T* get() const {
            return boost::get<T>(&_var);
        }

        //////////////////////////////////////////////////////////////////////////

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const;
        template<class Archive>
        void load(Archive & ar, const unsigned int version);

        //////////////////////////////////////////////////////////////////////////


        explicit Item(Real val) : _var(val) {}
        explicit Item(double val) : _var((Real)val) {}
        explicit Item(SInt32 val) : _var(val) {}
        explicit Item(int val) : _var((SInt32)val) {}
        explicit Item(bool val) : _var((SInt32)val) {}
        explicit Item(FormId id) : _var(id) {}
        explicit Item(object_base& o) : _var(o) {}

        explicit Item(const std::string& val) : _var(val) {}
        explicit Item(std::string&& val) : _var(val) {}

        // the Item is none if the pointers below are zero:
        explicit Item(const TESForm *val) {
            *this = val;
        }
        explicit Item(const char * val) {
            *this = val;
        }
        explicit Item(const BSFixedString& val) {
            *this = val.data;
        }
        explicit Item(object_base *val) {
            *this = val;
        }
        explicit Item(const object_stack_ref &val) {
            *this = val.get();
        }
/*
        explicit Item(const BSFixedString& val) : _var(boost::blank()) {
            *this = val.data;
        }*/

        Item& operator = (unsigned int val) { _var = (SInt32)val; return *this;}
        Item& operator = (int val) { _var = (SInt32)val; return *this;}
        Item& operator = (bool val) { _var = (SInt32)val; return *this;}
        Item& operator = (SInt32 val) { _var = val; return *this;}
        Item& operator = (Real val) { _var = val; return *this; }
        Item& operator = (double val) { _var = (Real)val; return *this; }
        Item& operator = (const std::string& val) { _var = val; return *this;}
        Item& operator = (std::string&& val) { _var = val; return *this;}
        Item& operator = (boost::blank) { _var = boost::blank(); return *this; }
        Item& operator = (boost::none_t) { _var = boost::blank(); return *this; }
        Item& operator = (object_base& v) { _var = &v; return *this; }


        Item& operator = (FormId formId) {
            // prevent zero FormId from being saved
            if (formId) {
                _var = formId;
            } else {
                _var = blank();
            }
            return *this;
        }

        template<class T>
        Item& _assignPtr(T *ptr) {
            if (ptr) {
                _var = ptr;
            } else {
                _var = blank();
            }
            return *this;
        }

        Item& operator = (const char *val) {
            return _assignPtr(val);
        }

        Item& operator = (object_base *val) {
            return _assignPtr(val);
        }

        Item& operator = (const TESForm *val) {
            if (val) {
                _var = (FormId)val->formID;
            } else {
                _var = blank();
            }
            return *this;
        }

        object_base *object() const {
            if (auto ref = boost::get<internal_object_ref>(&_var)) {
                return ref->get();
            }
            return nullptr;
        }

        Real fltValue() const {
            if (auto val = boost::get<Item::Real>(&_var)) {
                return *val;
            }
            else if (auto val = boost::get<SInt32>(&_var)) {
                return *val;
            }
            return 0;
        }

        SInt32 intValue() const {
            if (auto val = boost::get<SInt32>(&_var)) {
                return *val;
            }
            else if (auto val = boost::get<Item::Real>(&_var)) {
                return *val;
            }
            return 0;
        }

        const char * strValue() const {
            if (auto val = boost::get<std::string>(&_var)) {
                return val->c_str();
            }
            return nullptr;
        }

        std::string * stringValue() {
            return boost::get<std::string>(&_var);
        }

        TESForm * form() const {
            auto frmId = formId();
            return frmId != FormZero ? skse::lookup_form(frmId) : nullptr;
        }

        FormId formId() const {
            if (auto val = boost::get<FormId>(&_var)) {
                return *val;
            }
            return FormZero;
        }

        class are_strict_equals : public boost::static_visitor<bool> {
        public:

            template <typename T, typename U>
            bool operator()( const T &, const U & ) const {
                return false; // cannot compare different types
            }

            template <typename T>
            bool operator()( const T & lhs, const T & rhs ) const {
                return lhs == rhs;
            }

            template <> bool operator()(const std::string & lhs, const std::string & rhs) const {
                return _stricmp(lhs.c_str(), rhs.c_str()) == 0;
            }
        };

        bool isEqual(const Item& other) const {
            return boost::apply_visitor(are_strict_equals(), _var, other._var);
        }

        bool isNull() const {
            return is_type<boost::blank>();
        }

        bool isNumber() const {
            return is_type<SInt32>() || is_type<Real>();
        }

        template<class T> T readAs();

        //////////////////////////////////////////////////////////////////////////
    private:
        static_assert(std::is_same<
            boost::variant<boost::blank, SInt32, Real, FormId, internal_object_ref, std::string>,
            variant
        >::value, "update _user2variant code below");

        // maps input user type to variant type:
        template<class T> struct _user2variant { using variant_type = T; };
        template<class V> struct _variant_type { using variant_type = V; };

        template<> struct _user2variant<uint32_t> : _variant_type<SInt32>{};
        template<> struct _user2variant<int32_t> : _variant_type<SInt32>{};
        template<> struct _user2variant<bool> : _variant_type<SInt32>{};

        template<> struct _user2variant<double> : _variant_type<Real>{};

        template<> struct _user2variant<const char*> : _variant_type<std::string>{};

        //template<> struct _user2variant<object_base> : _variant_type<internal_object_ref>{};
        template<> struct _user2variant<const object_base*> : _variant_type<internal_object_ref>{};

    public:

        bool operator == (const Item& other) const { return isEqual(other);}
        bool operator != (const Item& other) const { return !isEqual(other);}

        bool operator == (const object_base &obj) const { return *this == &obj; }

		template<class T>
		bool operator == (const T& v) const {
            auto thisV = boost::get<typename _user2variant<T>::variant_type >(&_var);
            return thisV && *thisV == v;
        }

        template<class T>
        bool operator != (const T& v) const { return !(*this == v); }

		//////////////////////////////////////////////////////////////////////////

        bool operator < (const Item& other) const {
            const auto l = type(), r = other.type();
            return l == r ? boost::apply_visitor(lesser_comparison(), _var, other._var) : (l < r);
        }
    private:
        class lesser_comparison : public boost::static_visitor < bool > {
        public:

            template <typename T, typename U>
            bool operator()(const T &, const U &) const {
                return type2index<T>::index < type2index<U>::index;
            }

            template <typename T>
            bool operator()(const T & lhs, const T & rhs) const {
                return lhs < rhs;
            }

            bool operator()(const std::string & lhs, const std::string & rhs) const {
                return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
            }

        };

    };

    template<> inline Item::Real Item::readAs<Item::Real>() {
        return fltValue();
    }

    template<> inline SInt32 Item::readAs<SInt32>() {
        return intValue();
    }

    template<> inline const char * Item::readAs<const char *>() {
        return strValue();
    }

    template<> inline std::string Item::readAs<std::string>() {
        auto str = stringValue();
        return str ? *str : std::string();
    }

    template<> inline BSFixedString Item::readAs<BSFixedString>() {
        const char *chr = strValue();
        return chr ? BSFixedString(chr) : BSFixedString();
    }

    template<> inline Handle Item::readAs<Handle>() {
        auto obj = object();
        return obj ? obj->uid() : HandleNull;
    }

    template<> inline TESForm * Item::readAs<TESForm*>() {
        return form();
    }

    template<> inline object_base * Item::readAs<object_base*>() {
        return object();
    }

    class array : public collection_base< array >
    {
        array(const array&);
        array& operator=(const array&);

    public:

        array() {}

        enum {
            TypeId = CollectionType::Array,
        };

        typedef SInt32 Index;

        typedef std::vector<Item> container_type;
        typedef container_type::iterator iterator;
        typedef container_type::reverse_iterator reverse_iterator;

        container_type _array;

        container_type& u_container() {
            return _array;
        }

        const container_type& u_container() const {
            return _array;
        }

        container_type container_copy() const {
            object_lock g(this);
            return _array;
        }

        template<class T> void push(T&& item) {
            object_lock g(this);
            u_push(item);
        }

        template<class T> void u_push(T&& item) {
            _array.emplace_back(std::forward<T>(item));
        }

        void u_clear() override {
            _array.clear();
        }

        SInt32 u_count() const override {
            return _array.size();
        }

        void u_nullifyObjects() override;

        void u_visit_referenced_objects(const std::function<void(object_base&)>& visitor) override {
            for (auto& item : _array) {
                if (auto obj = item.object()) {
                    visitor(*obj);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////

        boost::optional<int32_t> u_convertIndex(int32_t pyIndex) const {
            int32_t count = (int32_t)_array.size();
            int32_t index = (pyIndex >= 0 ? pyIndex : (count + pyIndex));
            return{ index >= 0 && index < count, index };
        }

        Item* u_getItem(int32_t index) {
            auto idx = u_convertIndex(index);
            return idx ? &_array[*idx] : nullptr;
        }

        void setItem(int32_t index, const Item& itm) {
            object_lock g(this);
            auto idx = u_convertIndex(index);
            if (idx) {
                _array[*idx] = itm;
            }
        }

        Item& operator [] (int32_t index) { return const_cast<Item&>(const_cast<const array*>(this)->operator[](index)); }
        const Item& operator [] (int32_t index) const {
            auto idx = u_convertIndex(index);
            assert(idx);
            return _array[*idx];
        }

        Item getItem(int32_t index) {
            object_lock lock(this);
            auto itm = u_getItem(index);
            return itm ? *itm : Item();
        }

        iterator begin() { return _array.begin();}
        iterator end() { return _array.end(); }

        reverse_iterator rbegin() { return _array.rbegin();}
        reverse_iterator rend() { return _array.rend(); }


        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };

    template<class RealType, class ContainerType>
    class basic_map_collection : public collection_base< RealType > {
    public:
        typedef ContainerType container_type;
        typedef typename ContainerType::key_type key_type;
    protected:
        ContainerType cnt;

    public:

        const container_type& u_container() const {
            return cnt;
        }

        container_type& u_container() {
            return cnt;
        }

        container_type container_copy() {
            object_lock g(this);
            return cnt;
        }

        Item findOrDef(const key_type& key) {
            object_lock g(this);
            auto result = u_find(key);
            return result ? *result : Item();
        }

        const Item* u_find(const key_type& key) const {
            auto itr = cnt.find(key);
            return itr != cnt.end() ? &(itr->second) : nullptr;
        }

        Item* u_find(const key_type& key) { return const_cast<Item*>( const_cast<const basic_map_collection*>(this)->u_find(key) ); }

        bool erase(const key_type& key) {
            object_lock g(this);
            return u_erase(key);
        }

        bool u_erase(const key_type& key) {
            auto itr = cnt.find(key);
            return itr != cnt.end() ? (cnt.erase(itr), true) : false;
        }

        void u_clear() override {
            cnt.clear();
        }

        template<class T> void u_setValueForKey(const key_type& key, T&& value) {
            cnt[key] = std::forward<T>(value);
        }

        template<class T>void setValueForKey(const key_type& key, T&& value) {
            object_lock g(this);
            u_setValueForKey(key, value);
        }

        SInt32 u_count() const override {
            return cnt.size();
        }

        Item& operator [] (const key_type& key) {
            return const_cast<Item&>(const_cast<const basic_map_collection&>(*this)[key]);
        }

        const Item& operator [] (const key_type& key) const {
            auto itm = u_find(key);
            assert(itm);
            return *itm;
        }
        
        void u_visit_referenced_objects(const std::function<void(object_base&)>& visitor) override {
            for (auto& pair : u_container()) {
                if (auto obj = pair.second.object()) {
                    visitor(*obj);
                }
            }
        }

        void u_nullifyObjects() override {
            for (auto& pair : u_container()) {
                pair.second.u_nullifyObject();
            }
        }
    };

    struct map_case_insensitive_comp {
        bool operator() (const std::string& lhs, const std::string& rhs) const {
            return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
        }
    };


    class map : public basic_map_collection< map, std::map<std::string, Item, map_case_insensitive_comp > >
    {
    public:
        enum  {
            TypeId = CollectionType::Map,
        };

        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };


    class form_map : public basic_map_collection< form_map, std::map<FormId, Item> >
    {
    public:
        enum  {
            TypeId = CollectionType::FormMap,
        };

        void u_onLoaded() override;

        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };

    class integer_map : public basic_map_collection < integer_map, std::map<int32_t, Item> >
    {
    public:
        enum  {
            TypeId = CollectionType::IntegerMap,
        };

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };
}
