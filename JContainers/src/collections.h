#pragma once

#include "common/ITypes.h"
#include "common/IDebugLog.h"

#include "skse/PapyrusVM.h"

#include <vector>
#include <string>
#include <assert.h>
//#include <mutex>

#include <boost/serialization/split_member.hpp>

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

        static T* create(tes_context& context = tes_context::instance()) {
            auto obj = new T();
            obj->_context = &context;
            obj->_registerSelf();
            return obj;
        }

        template<class Init>
        static T* _createWithInitializer(Init& init, tes_context& context = tes_context::instance()) {
            auto obj = new T();
            obj->_context = &context;
            init(obj);
            obj->_registerSelf();
            return obj;
        }

        static T* object(tes_context& context = tes_context::instance()) {
            return static_cast<T *> (create(context)->autorelease());
        }

        template<class Init>
        static T* objectWithInitializer(Init& init, tes_context& context = tes_context::instance()) {
            return static_cast<T *> (_createWithInitializer(init, context)->autorelease());
        }
    };

    enum ItemType : unsigned char
    {
        ItemTypeNone = 0,
        ItemTypeInt32 = 1,
        ItemTypeFloat32 = 2,
        ItemTypeCString = 3,
        ItemTypeObject = 4,
        ItemTypeForm = 5,
    };

    enum FormId : UInt32 {
        FormZero = 0,
    };


    struct StringMem
    {
        UInt32 bufferSize;
        char string[1];

        static StringMem* allocWithSize(UInt32 aBufferSize) {
            UInt32 memSize = (aBufferSize + 1) + sizeof(StringMem) - 1;
            StringMem *me = (StringMem *)operator new(memSize);
            me->bufferSize = aBufferSize + 1;
            me->string[0] = '\0';
            return me;
        }

        static StringMem* allocWithString(const char* string) {
            StringMem *me = allocWithSize(string ? strlen(string) : 0);
            if (string)
                strcpy_s(me->string, me->bufferSize, string);
            return me;
        }

        /*static StringMem* null() {
            static StringMem nullObj = {1, '\0'};
            return &nullObj;
        }*/

        void setEmpty() {
            string[0] = '\0';
        }

        UInt32 length () const {
            return strlen(string);
        }
    };

    class array;
    class map;
    class object_base;

    class Item
    {
        union {
            UInt32 _formId;
            SInt32 _intVal;
            Float32 _floatVal;
            StringMem *_stringVal;
            object_base *_object;
        };

        ItemType _type;

    public:

        Item() : _stringVal(nullptr), _type(ItemTypeNone) {}

        explicit Item(Float32 val) : _floatVal(val), _type(ItemTypeFloat32) {}
        explicit Item(double val) : _floatVal(val), _type(ItemTypeFloat32) {}
        explicit Item(SInt32 val) : _intVal(val), _type(ItemTypeInt32) {}
        explicit Item(int val) : _intVal(val), _type(ItemTypeInt32) {}
        explicit Item(bool val) : _intVal(val), _type(ItemTypeInt32) {}

        explicit Item(const TESForm *form) : _formId(form ? form->formID : 0), _type(ItemTypeForm) {}
        explicit Item(FormId id) : _formId(id), _type(ItemTypeForm) {}

        explicit Item(const char * val) : _stringVal(NULL), _type(ItemTypeNone) {
            setStringVal(val);
        }

        explicit Item(const std::string& val) : _stringVal(NULL), _type(ItemTypeNone) {
            setStringVal(val.c_str());
        }

        explicit Item(const BSFixedString& str) : _stringVal(NULL), _type(ItemTypeNone) {
            setStringVal(str.data);
        }

        explicit Item(object_base *collection) : _stringVal(NULL), _type(ItemTypeNone) {
            setObjectVal(collection);
        }

        /*explicit Item(Handle hdl) : _stringVal(NULL), _type(ItemTypeNone) {
            auto obj = collection_registry::getObject(hdl);
            if (obj)
                setObjectVal(obj);
        }*/

/*
        explicit Item(HandleT hdl) : _stringVal(NULL), _type(ItemTypeNone) {
            auto obj = collection_registry::getObject(hdl);
            if (obj)
                setObjectVal(obj);
        }*/

        bool operator == (const Item& other) const {
            return _type == other._type && _object == other._object;
        }

        bool operator != (const Item& other) const {
            return !(*this == other);
        }

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const;
        template<class Archive>
        void load(Archive & ar, const unsigned int version);

        void setFlt(float val) {
            _freeObject();
            _freeString();

            _type = ItemTypeFloat32;
            _floatVal = val;
        }

        void setInt(SInt32 val) {
            _freeObject();
            _freeString();

            _type = ItemTypeInt32;
            _intVal = val;
        }

        ItemType type() const { return _type;}

        void setStringVal(const char *val) {

            _freeObject();

            StringMem *block = (_type == ItemTypeCString ? _stringVal : 0);
            _type = ItemTypeCString;

            if (!val) {
                if (block) {
                    block->setEmpty();
                } else {
                    _stringVal = nullptr;
                }
            } else {
                int lenPlusEnd = strlen(val);

                if (block && block->bufferSize >= lenPlusEnd) {
                    strcpy_s(block->string, block->bufferSize, val);
                } else {
                    delete block;
                    _stringVal = StringMem::allocWithString(val);
                }
            }
        }

        void setObjectVal(object_base *obj) {
            if (object() == obj)
                return;

            _replaceObject(obj);
        }

        void u_nullifyObject() {
            if (object()) {
                _object = nullptr;
                _type = ItemTypeNone;
            }
        }

        bool _freeObject() {
            object_base *prev = object();
            if (prev) {
                prev->release();
                _object = nullptr;
                return true;
            }

            return false;
        }

        bool _freeString() {
            if (_type == ItemTypeCString) {
                delete _stringVal;
                _stringVal = nullptr;
                return true;
            }

            return false;
        }

        void _replaceObject(object_base *obj) {
            if (!_freeObject())
                _freeString();

            _type = ItemTypeObject;
            _object = obj;

            if (obj)
                obj->retain();
        }

        ~Item() {
            if (_freeString()) {
                ;
            }
            else if (_freeObject()) {
                ;
            }
        }

        Item(Item&& other) : _stringVal(other._stringVal), _type(other._type) {
            other._stringVal = nullptr;
            other._type = ItemTypeNone;
        }

        Item& operator=(Item&& other) {
            _freeString();
            _freeObject();

            _type = other._type;
            _stringVal = other._stringVal;

            other._stringVal = nullptr;
            other._type = ItemTypeNone;

            return *this;
        }

        Item(const Item& other) : _stringVal(nullptr), _type(ItemTypeNone) {
            _copy(other);
        }

        Item& operator=(const Item& other) {
            _copy(other);
            return *this;
        }

        void _copy(const Item &other) {
            if (other._type == ItemTypeCString) {
                setStringVal(other.strValue());
            } else if (other.object()){
                setObjectVal(other.object());
            } else {
                _type = other._type;
                _stringVal = other._stringVal;
            }
        }

        Float32 fltValue() const {
            return _type == ItemTypeFloat32 ? _floatVal : (_type == ItemTypeInt32 ? _intVal : 0);
        }

        SInt32 intValue() const {
            return _type == ItemTypeInt32 ? _intVal : (_type == ItemTypeFloat32 ? _floatVal : 0);
        }

        const char * strValue() const {
            return (_type == ItemTypeCString && _stringVal) ? _stringVal->string : 0;
        }

        TESForm * form() const {
            return _type == ItemTypeForm ? LookupFormByID(_formId) : nullptr;
        }

        FormId formId() const {
            return (FormId) (_type == ItemTypeForm ? _formId : 0);
        }

        void setForm(const TESForm *form) {
            setFormId((FormId)(form ? form->formID : 0));
        }

        void setFormId(FormId formId) {
            _freeString();
            _freeObject();

            _type = ItemTypeForm;
            _formId = formId;
        }

        bool isEqual(SInt32 value) const {
            return _type == ItemTypeInt32 && _intVal == value;
        }

        bool isEqual(Float32 value) const {
            return _type == ItemTypeFloat32 && _floatVal == value;
        }

        bool isEqual(const char* value) const {
            auto str1 = strValue();
            auto str2 = value;
            return  _type == ItemTypeCString && (str1 && str2 && strcmp(str1, str2) == 0) || (!str1 && !str2);
        }

        bool isEqual(const object_base *value) const {
            return _type == ItemTypeObject && _object == value;
        }

        bool isEqual(const TESForm *value) const {
            return _type == ItemTypeForm && _formId == (value ? value->formID : 0);
        }

        bool isEqual(const Item& other) const {
            if (type() != other.type()) {
                return false;
            }

            switch(type()) {
            case ItemTypeInt32:
                return _intVal == other._intVal;
            case ItemTypeFloat32:
                return _floatVal == other._floatVal;
            case ItemTypeForm:
                return _formId == other._formId;
            case ItemTypeObject:
                return _object == other._object;
            case ItemTypeCString:
                return isEqual(other.strValue());
            default:
                return true;
            }
        }

        bool isNull() const {
            return _type == ItemTypeNone;
        }

        bool isNumber() const {
            return _type == ItemTypeFloat32 || _type == ItemTypeInt32;
        }

        object_base *object() const {
            return (_type == ItemTypeObject) ? _object : nullptr;
        }

        template<class T> T readAs();
        template<class T> void writeAs(T val);
    };

    template<> inline float Item::readAs<Float32>() {
        return fltValue();
    }

    template<> inline void Item::writeAs(Float32 val) {
        setFlt(val);
    }

    template<> inline SInt32 Item::readAs<SInt32>() {
        return intValue();
    }

    template<> inline void Item::writeAs(SInt32 val) {
        setInt(val);
    }

    template<> inline const char * Item::readAs<const char *>() {
        return strValue();
    }

    template<> inline void Item::writeAs(const char * val) {
        setStringVal(val);
    }

    template<> inline void Item::writeAs(BSFixedString val) {
        setStringVal(val.data);
    }

    template<> inline BSFixedString Item::readAs<BSFixedString>() {
        const char *chr = strValue();
        return chr ? BSFixedString(chr) : nullptr;
    }

    template<> inline Handle Item::readAs<Handle>() {
        auto obj = object();
        return obj ? obj->id : HandleNull;
    }

    template<> inline TESForm * Item::readAs<TESForm*>() {
        return form();
    }

    template<> inline void Item::writeAs(TESForm *form) {
        return setForm(form);
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

        const container_type& container() const {
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

        const container_type& container() const {
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
