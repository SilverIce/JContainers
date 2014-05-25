#pragma once

#include "common/ITypes.h"
#include "common/IDebugLog.h"

#include "skse/PapyrusVM.h"

#include <vector>
#include <string>
#include <assert.h>
//#include <mutex>

#include <boost/serialization/split_member.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/variant.hpp>

#include "skse/GameForms.h"

#include "object_base.h"

#include "tes_context.h"

namespace collections {


    template<class T>
    class collection_base_T : public object_base
    {
    protected:

        explicit collection_base_T() : object_base((CollectionType)T::TypeId) {}

    public:

        static T* create(tes_context& context /*= tes_context::instance()*/) {
            auto obj = new T();
            obj->_context = &context;
            obj->_registerSelf();
            return obj;
        }

        template<class Init>
        static T* _createWithInitializer(Init& init, tes_context& context /*= tes_context::instance()*/) {
            auto obj = new T();
            obj->_context = &context;
            init(obj);
            obj->_registerSelf();
            return obj;
        }

        static T* object(tes_context& context /*= tes_context::instance()*/) {
            return static_cast<T *> (create(context)->autorelease());
        }

        template<class Init>
        static T* objectWithInitializer(Init& init, tes_context& context /*= tes_context::instance()*/) {
            return static_cast<T *> (_createWithInitializer(init, context)->autorelease());
        }
    };

    enum FormId : UInt32 {
        FormZero = 0,
        FormGlobalPrefix = 0xFF,
    };

    class array;
    class map;
    class object_base;

    typedef boost::intrusive_ptr<object_base> object_ref;

    inline void intrusive_ptr_add_ref(object_base * p) {
        p->retain();
    }

    inline void intrusive_ptr_release(object_base * p) {
        p->release();
    }

    enum ItemType : unsigned char
    {
        ItemTypeNone = 0,
        ItemTypeInt32 = 1,
        ItemTypeFloat32 = 2,
        ItemTypeCString = 3,
        ItemTypeObject = 4,
        ItemTypeForm = 5,
    };

    class Item
    {
        typedef boost::blank blank;
        typedef boost::variant<boost::blank, SInt32, Float32, std::string, object_ref, FormId> variant;
        variant _var;

    public:

        void u_nullifyObject() {
            if (auto ref = boost::get<object_ref>(&_var)) {
                ref->jc_nullify();
            }
        }

        Item() : _var(boost::blank()) {}
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

        ItemType type() const { return (ItemType)_var.which();}

        //////////////////////////////////////////////////////////////////////////

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const;
        template<class Archive>
        void load(Archive & ar, const unsigned int version);

        //////////////////////////////////////////////////////////////////////////

        template<class T> static T cast_in(T& t) { return t;}

        //static T cast_in(T t) { return t;}

        explicit Item(Float32 val) : _var(val) {}
        explicit Item(double val) : _var((Float32)val) {}
        explicit Item(SInt32 val) : _var(val) {}
        explicit Item(int val) : _var((SInt32)val) {}
        explicit Item(bool val) : _var((SInt32)val) {}
        explicit Item(FormId id) : _var(id) {}

        explicit Item(const std::string& val) : _var(val) {}
        explicit Item(std::string&& val) : _var(val) {}

        // these are none if data pointers zero
        explicit Item(const TESForm *val) : _var(boost::blank()) {
            *this = val;
        }
        explicit Item(const char * val) : _var(boost::blank()) {
            *this = val;
        }
        explicit Item(object_base *val) : _var(boost::blank()) {
            *this = val;
        }
        explicit Item(const BSFixedString& val) : _var(boost::blank()) {
            *this = val.data;
        }

        Item& operator = (unsigned int val) { _var = (SInt32)val; return *this;}
        Item& operator = (int val) { _var = (SInt32)val; return *this;}
        //Item& operator = (bool val) { _var = (SInt32)val; return *this;}
        Item& operator = (SInt32 val) { _var = val; return *this;}
        Item& operator = (Float32 val) { _var = val; return *this;}
        Item& operator = (double val) { _var = (Float32)val; return *this;}
        Item& operator = (const std::string& val) { _var = val; return *this;}
        Item& operator = (std::string&& val) { _var = val; return *this;}


        // prevent form id be saved like integral number
        Item& operator = (FormId formId) {
            _var = formId;
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
            if (auto ref = boost::get<object_ref>(&_var)) {
                return ref->get();
            }
            return nullptr;
        }

        Float32 fltValue() const {
            if (auto val = boost::get<Float32>(&_var)) {
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
            else if (auto val = boost::get<Float32>(&_var)) {
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

        TESForm * form() const {
            auto frmId = formId();
            return frmId != FormZero ? LookupFormByID(frmId) : nullptr;
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
            return type() == ItemTypeInt32 && intValue() == value;
        }

        bool isEqual(Float32 value) const {
            return type() == ItemTypeFloat32 && fltValue() == value;
        }

        bool isEqual(const char* value) const {
            auto str1 = strValue();
            auto str2 = value;
            return type() == ItemTypeCString && ( (str1 && str2 && strcmp(str1, str2) == 0) || (!str1 && !str2) );
        }

        bool isEqual(const object_base *value) const {
            auto obj = object();
            return type() == ItemTypeObject && ( (obj && value && *obj == *value) || obj == value);
        }

        bool isEqual(const TESForm *value) const {
            return type() == ItemTypeForm && formId() == (value ? value->formID : 0);
        }

        bool isEqual(const Item& other) const {
            return boost::apply_visitor(are_strict_equals(), _var, other._var);
        }

        bool isNull() const {
            return type() == ItemTypeNone;
        }

        bool isNumber() const {
            auto kind = type();
            return kind == ItemTypeFloat32 || kind == ItemTypeInt32;
        }

        template<class T> T readAs();
    };

    template<> inline float Item::readAs<Float32>() {
        return fltValue();
    }

    template<> inline SInt32 Item::readAs<SInt32>() {
        return intValue();
    }

    template<> inline const char * Item::readAs<const char *>() {
        return strValue();
    }

    template<> inline BSFixedString Item::readAs<BSFixedString>() {
        const char *chr = strValue();
        return chr ? BSFixedString(chr) : nullptr;
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

    class array : public collection_base_T< array >
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

        void u_push(const Item& item) {
            _array.push_back(item);
        }

        void u_clear() override {
            _array.clear();
        }

        SInt32 u_count() override {
            return _array.size();
        }

        void u_nullifyObjects() override;

        Item* u_getItem(size_t index) {
            return index < _array.size() ? &_array[index] : nullptr;
        }

        iterator begin() { return _array.begin();}
        iterator end() { return _array.end(); }

        reverse_iterator rbegin() { return _array.rbegin();}
        reverse_iterator rend() { return _array.rend(); }

        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };

    class map : public collection_base_T< map >
    {
    public:

        enum  {
            TypeId = CollectionTypeMap,
        };

        struct comp { 
            bool operator() (const std::string& lhs, const std::string& rhs) const {
                return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
            }
        };

        typedef std::map<std::string, Item, comp> container_type;

    private:
        container_type cnt;

    public:

        const container_type& u_container() const {
            return cnt;
        }

        container_type container_copy() {
            object_lock g(this);
            return cnt;
        }

/*
        static std::string lowerString(const std::string& key) {
            std::string lowerKey(key);
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            return lowerKey;
        }*/

        container_type::iterator findItr(const std::string& key) {
            return cnt.find(key);
        }

        Item* u_find(const std::string& key) {
            auto itr = findItr(key);
            return itr != cnt.end() ? &(itr->second) : nullptr;
        }

        bool u_erase(const std::string& key) {
            auto itr = findItr(key);
            return itr != cnt.end() ? (cnt.erase(itr), true) : false;
        }

/*
        Item& operator [] (const std::string& key) {
            return cnt[key];
        }*/

        void u_setValueForKey(const std::string& key, const Item& value) {
            cnt[key] = value;
        }

        void setValueForKey(const std::string& key, const Item& value) {
            object_lock g(this);
            cnt[key] = value;
        }

        void u_nullifyObjects() override;

        void u_clear() override {
            cnt.clear();
        }

        SInt32 u_count() override {
            return cnt.size();
        }

        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };


    class form_map : public collection_base_T< form_map >
    {
    public:
        enum  {
            TypeId = CollectionTypeFormMap,
        };

        typedef std::map<FormId, Item> container_type;

    private:
        container_type cnt;

    public:

        const container_type& u_container() const {
            return cnt;
        }

        container_type container_copy() {
            object_lock g(this);
            return cnt;
        }

        Item& operator [] (FormId key) {
            return cnt[key];
        }

        Item* u_find(FormId key) {
            auto itr = cnt.find(key);
            return itr != cnt.end() ? &(itr->second) : nullptr;
        }

        bool u_erase(FormId key) {
            auto itr = cnt.find(key);
            return itr != cnt.end() ? (cnt.erase(itr), true) : false;
        }

        void u_clear() {
            cnt.clear();
        }

        void u_setValueForKey(FormId key, const Item& value) {
            cnt[key] = value;
        }

        void setValueForKey(FormId key, const Item& value) {
            object_lock g(this);
            cnt[key] = value;
        }

        SInt32 u_count() override {
            return cnt.size();
        }

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
