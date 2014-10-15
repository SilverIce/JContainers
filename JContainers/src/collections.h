#pragma once

#include <vector>
#include <string>
#include <assert.h>

#include <boost/serialization/split_member.hpp>
#include <boost/variant.hpp>

#include "common/ITypes.h"
#include "common/IDebugLog.h"
#include "skse/GameForms.h"

#include "object_base.h"
#include "skse.h"

namespace collections {

	class tes_context;

    template<class T>
    class collection_base : public object_base
    {
    protected:

        //static_assert(std::is_base_of<collection_base<T>, T>::value, "");

        explicit collection_base() : object_base((CollectionType)T::TypeId) {}

    public:

        typedef typename object_stack_ref_template<T> ref;
        typedef typename object_stack_ref_template<const T> cref;

        static T* make(tes_context& context /*= tes_context::instance()*/) {
            auto obj = new T();
            obj->set_context(context.obj_context());
            obj->_registerSelf();
            return obj;
        }

        template<class Init>
        static T* _makeWithInitializer(Init& init, tes_context& context /*= tes_context::instance()*/) {
            auto obj = new T();
            obj->set_context(context.obj_context());
            init(obj);
            obj->_registerSelf();
            return obj;
        }

        static T* object(tes_context& context /*= tes_context::instance()*/) {
            return make(context);
        }

        template<class Init>
        static T* objectWithInitializer(Init& init, tes_context& context /*= tes_context::instance()*/) {
            return _makeWithInitializer(init, context);
        }
    };

    enum FormId : UInt32 {
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

        uint32_t type() const {
            return _var.which() + 1;
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

        explicit Item(const std::string& val) : _var(val) {}
        explicit Item(std::string&& val) : _var(val) {}

        // these are none if data pointers zero
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
        //Item& operator = (bool val) { _var = (SInt32)val; return *this;}
        Item& operator = (SInt32 val) { _var = val; return *this;}
        Item& operator = (Real val) { _var = val; return *this; }
        Item& operator = (double val) { _var = (Real)val; return *this; }
        Item& operator = (const std::string& val) { _var = val; return *this;}
        Item& operator = (std::string&& val) { _var = val; return *this;}
        Item& operator = (boost::blank) { _var = boost::blank(); return *this; }


        // prevent form id be saved like integral number
        Item& operator = (FormId formId) {
            if (formId) {
                _var = formId;
            } else {
                _var = blank();
            }
            return *this;
        }

        template<class T>
        Item& _assignToPtr(T *ptr) {
            if (ptr) {
                _var = ptr;
            } else {
                _var = blank();
            }
            return *this;
        }

        Item& operator = (const char *val) {
            return _assignToPtr(val);
        }

        Item& operator = (object_base *val) {
            return _assignToPtr(val);
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
        };

        bool isEqual(SInt32 value) const {
            return is_type<SInt32>() && intValue() == value;
        }

        bool isEqual(Real value) const {
            return is_type<Real>() && fltValue() == value;
        }

        bool isEqual(const char* value) const {
            auto str1 = strValue();
            auto str2 = value;
            return is_type<std::string>() && ( (str1 && str2 && strcmp(str1, str2) == 0) || (!str1 && !str2) );
        }

        bool isEqual(const object_base *value) const {
            return is_type<internal_object_ref>() && object() == value;
        }

        bool isEqual(const object_stack_ref& value) const {
            return isEqual(value.get());
        }

        bool isEqual(const TESForm *value) const {
            return is_type<FormId>() && formId() == (value ? value->formID : 0);
        }

        bool isEqual(const Item& other) const {
            return boost::apply_visitor(are_strict_equals(), _var, other._var);
        }

        bool operator == (const Item& other) const {
            return isEqual(other);
        }

        bool operator != (const Item& other) const {
            return !isEqual(other);
        }

        bool isNull() const {
            return is_type<boost::blank>();
        }

        bool isNumber() const {
            return is_type<SInt32>() || is_type<Real>();
        }

        template<class T> T readAs();
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

/*
    template<> inline BSFixedString Item::readAs<BSFixedString>() {
        const char *chr = strValue();
        return chr ? BSFixedString(chr) : nullptr;
    }*/


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
    public:

        enum {
            TypeId = CollectionTypeArray,
        };

        typedef SInt32 Index;

        typedef std::vector<Item> container_type;
        typedef container_type::iterator iterator;
        typedef container_type::reverse_iterator reverse_iterator;

        container_type _array;

        container_type& u_container() {
            return _array;
        }

        container_type container_copy() {
            object_lock g(this);
            return _array;
        }

        void push(const Item& item) {
            object_lock g(this);
            u_push(item);
        }

        void u_push(const Item& item) {
            _array.push_back(item);
        }

        void u_clear() override {
            _array.clear();
        }

        SInt32 u_count() const override {
            return _array.size();
        }

        void u_nullifyObjects() override;

        int32_t u_convertIndex(int32_t idx) const {
            return idx >= 0 ? idx : ((int32_t)_array.size() - idx);
        }

        Item* u_getItem(int32_t index) {
            auto idx = u_convertIndex(index);
            return idx < _array.size() ? &_array[idx] : nullptr;
        }

        void setItem(int32_t index, const Item& itm) {
            object_lock g(this);
            auto idx = u_convertIndex(index);
            if (idx < _array.size()) {
                _array[idx] = itm;
            }
        }

        Item getItem(int32_t index) {
            object_lock lock(this);
            auto idx = u_convertIndex(index);
            return idx < _array.size() ? _array[idx] : Item();
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

        Item find(const key_type& key) {
            object_lock g(this);
            auto result = u_find(key);
            return result ? *result : Item();
        }

        typename container_type::iterator findItr(const key_type& key) {
            return cnt.find(key);
        }

        Item* u_find(const key_type& key) {
            auto itr = findItr(key);
            return itr != cnt.end() ? &(itr->second) : nullptr;
        }

        bool erase(const key_type& key) {
            object_lock g(this);
            return u_erase(key);
        }

        bool u_erase(const key_type& key) {
            auto itr = findItr(key);
            return itr != cnt.end() ? (cnt.erase(itr), true) : false;
        }

        void u_clear() override {
            cnt.clear();
        }

        void u_setValueForKey(const key_type& key, const Item& value) {
            cnt[key] = value;
        }

        void setValueForKey(const key_type& key, const Item& value) {
            object_lock g(this);
            cnt[key] = value;
        }

        SInt32 u_count() const override {
            return cnt.size();
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
            TypeId = CollectionTypeMap,
        };

        void u_nullifyObjects() override;

        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };


    class form_map : public basic_map_collection< form_map, std::map<FormId, Item> >
    {
    public:
        enum  {
            TypeId = CollectionTypeFormMap,
        };

/*
        Item& operator [] (FormId key) {
            return cnt[key];
        }
*/

        void u_onLoaded() override {
            u_updateKeys();
        }

        // may call release if key is invalid
        // may also produces retain calls
        void u_updateKeys();

        void u_nullifyObjects() override;

        //////////////////////////////////////////////////////////////////////////

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const;
        template<class Archive>
        void load(Archive & ar, const unsigned int version);
    };
}
