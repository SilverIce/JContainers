#pragma once

#include "common/ITypes.h"
#include "common/IDebugLog.h"

#include "skse/GameTypes.h"
#include "skse/PapyrusVM.h"

#include <vector>
#include <hash_map>
#include <string>
#include <assert.h>
#include <atomic>

#include <boost/serialization/split_member.hpp>

#include <mutex>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/thread/lock_guard.hpp>

#include "id_generator.h"
#include "skse/GameForms.h"

class VMClassRegistry;
struct StaticFunctionTag;
class T_Base;

namespace collections {

    typedef std::recursive_mutex object_mutex;
    typedef std::lock_guard<object_mutex> mutex_lock;

    typedef boost::shared_mutex bshared_mutex;
    typedef boost::lock_guard<bshared_mutex> write_lock;
    typedef boost::shared_lock_guard<bshared_mutex> read_lock;

    typedef UInt32 HandleT;

    enum Handle : HandleT {
        HandleNull = 0,
    };

    class object_base;

    enum CollectionType
    {
        CollectionTypeNone = 0,
        CollectionTypeArray,
        CollectionTypeMap,
        CollectionTypeFormMap,
    };

    class collection_registry
    {
    private:

        friend class shared_state;

        typedef std::map<HandleT, object_base *> registry_container;

        std::map<HandleT, object_base *> _map;
        id_generator<HandleT> _idGen;
        bshared_mutex& _mutex;
        int objectsCreated;
        int objectDestroyed;

        collection_registry(const collection_registry& );
        collection_registry& operator = (const collection_registry& );
         
    public:

        explicit collection_registry(bshared_mutex &mt)
            : _mutex(mt)
        {
        }

        static Handle registerObject(object_base *collection);

        static void removeObject(HandleT hdl) {
            if (!hdl) {
                return;
            }
            
            collection_registry& me = instance();
            write_lock g(me._mutex);
            auto itr = me._map.find(hdl);
            assert(itr != me._map.end());
            me._map.erase(itr);
            me._idGen.reuseId(hdl);
        }

        static object_base *getObject(HandleT hdl) {
            if (!hdl) {
                return nullptr;
            }
            
            collection_registry& me = instance();
            read_lock g(me._mutex);
            auto itr = me._map.find(hdl);
            if (itr != me._map.end())
                return itr->second;

            return nullptr;
        }

        template<class T>
        static T *getObjectOfType(HandleT hdl) {
            auto obj = getObject(hdl);
            return (obj && obj->_type == T::TypeId) ? (T*)obj : nullptr;
        }

        static collection_registry& instance();

        void _clear();

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };

    class object_base
    {
        int32_t _refCount;
        int32_t _tes_refCount;

    public:

        virtual ~object_base() {}

    public:
        object_mutex _mutex;
        union {
            Handle id;
            UInt32 _id;
        };

        CollectionType _type;

        explicit object_base(CollectionType type)
            : _refCount(1)
            , _tes_refCount(0)
            , id(HandleNull)
            , _type(type)
        {
        }

        object_base()
            : _refCount(0)
            , _tes_refCount(0)
            , _id(0)
            , _type(CollectionTypeNone)
        {
        }

        int refCount() {
            mutex_lock g(_mutex);
            return _refCount + _tes_refCount;
        }

        bool registered() const {
            return _id != 0;
        }

        template<class T> T* as() {
            return T::TypeId == _type ? static_cast<T*>(this) : nullptr;
        }

        object_base * retain() {
            mutex_lock g(_mutex);
            ++_refCount;
            return this;
        }

        object_base * tes_retain() {
            mutex_lock g(_mutex);
            ++_tes_refCount;
            return this;
        }

        object_base * autorelease();

        void release() {
            bool deleteObject = false; {
                mutex_lock g(_mutex);
                if (_refCount > 0) {
                    --_refCount;
                    deleteObject = (_refCount == 0 && _tes_refCount == 0);
                }
            }

            if (deleteObject) {
                collection_registry::removeObject(id);
                delete this;
            }
        }

        void tes_release() {
            bool deleteObject = false; {
                mutex_lock g(_mutex);
                if (_tes_refCount > 0) {
                    --_tes_refCount;
                    deleteObject = (_refCount == 0 && _tes_refCount == 0);
                }
            }

            if (deleteObject) {
                collection_registry::removeObject(id);
                delete this;
            }
        }

        virtual void clear() = 0;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };

    inline Handle collection_registry::registerObject(object_base *collection) {
        if (collection->registered()) {
            return collection->id;
        }

        collection_registry& me = instance();
        write_lock g(me._mutex);
        auto newId = me._idGen.newId();
        collection->_id = newId;
        assert(me._map.find(newId) == me._map.end());
        me._map[newId] = collection;
        return (Handle)newId;
    }

    inline void collection_registry::_clear() {
        /*  Not good, but working solution.

            issue: deadlock during loading savegame - e.g. cleaning current state.

            due to: collection release -> dealloc -> collection_registry::removeObject
                 introduces deadlock ( registry locked during _clear call)

            solution: +1 refCount to all objects & clear & delete them

            all we need is just free the memory but this will require track allocated collection & stl memory blocks 
        */
        for (auto& pair : _map) {
            pair.second->retain(); // to guarant that removeObject will not be called during clear method call
        }
        for (auto& pair : _map) {
            pair.second->clear(); // to force ~Item() calls while all collections alive (~Item() may release collection)
        }
        for (auto& pair : _map) {
            delete pair.second;
        }
        _map.clear();
    }

    template<class Val>
    struct Tes2Value
    {
        typedef Val tes_type;
    };

    template<>
    struct Tes2Value<Handle>
    {
        typedef HandleT tes_type;
    };

    template<class T>
    class collection_base_T : public object_base
    {
    protected:

        collection_base_T() : object_base((CollectionType)T::TypeId) {}

    public:

        static T* create() {
            auto obj = new T();
            collection_registry::registerObject(obj);
            return obj;
        }

        template<class Init>
        static T* createWithInitializer(Init init) {
            auto obj = new T();
            init(obj);
            collection_registry::registerObject(obj);
            return obj;
        }

        static T* object() {
            return static_cast<T *> (create()->autorelease());
        }

        template<class Init>
        static T* objectWithInitializer(Init init) {
            return static_cast<T *> (createWithInitializer(init)->autorelease());
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

    struct StringMem
    {
        UInt32 size;
        char string[1];

        static StringMem* allocWithSize(UInt32 size) {
            UInt32 memSize = size + 1 + sizeof(StringMem) - 1;
            StringMem *me = (StringMem *)operator new(memSize);
            me->size = memSize;
            me->string[0] = '\0';
            return me;
        }

        static StringMem* allocWithString(const char* string) {
            StringMem *me = allocWithSize(string ? strlen(string) : 0);
            if (string)
                strcpy_s(me->string, me->size, string);
            return me;
        }

        static StringMem* null() {
            static StringMem nullObj = {0, '\0'};
            return &nullObj;
        }

        void setEmpty() {
            string[0] = '\0';
        }

        UInt32 length () const {
            return strlen(string);
        }
    };

    class array;
    class map;

    class Item
    {
        union {
            UInt32 _uintVal;
            SInt32 _intVal;
            Float32 _floatVal;
            StringMem *_stringVal;
            object_base *_object;
            array *_array;
            map* _map;
        };

        ItemType _type;

    public:

        Item() : _stringVal(nullptr), _type(ItemTypeNone) {}

        explicit Item(Float32 val) : _floatVal(val), _type(ItemTypeFloat32) {}
        explicit Item(double val) : _floatVal(val), _type(ItemTypeFloat32) {}
        explicit Item(SInt32 val) : _intVal(val), _type(ItemTypeInt32) {}
        explicit Item(int val) : _intVal(val), _type(ItemTypeInt32) {}
        explicit Item(bool val) : _intVal(val), _type(ItemTypeInt32) {}
        explicit Item(const TESForm *form) : _uintVal(form ? form->formID : 0), _type(ItemTypeForm) {}

        explicit Item(const char * val) : _stringVal(NULL), _type(ItemTypeNone) {
            setStringVal(val);
        }

        explicit Item(const BSFixedString& str) : _stringVal(NULL), _type(ItemTypeNone) {
            setStringVal(str.data);
        }

        explicit Item(object_base *collection) : _stringVal(NULL), _type(ItemTypeNone) {
            setObjectVal(collection);
        }

        explicit Item(Handle hdl) : _stringVal(NULL), _type(ItemTypeNone) {
            auto obj = collection_registry::getObject(hdl);
            if (obj)
                setObjectVal(obj);
        }

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
                int len = strlen(val);

                if (block && block->size >= len) {
                    strcpy_s(block->string, block->size, val);
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
            return _type == ItemTypeForm ? LookupFormByID(_uintVal) : nullptr;
        }

        void setForm(const TESForm *form) {
            _freeString();
            _freeObject();

            _type = ItemTypeForm;
            _uintVal = (form ? form->formID : 0);
        }

        bool isNull() const {
            return _type == ItemTypeNone;
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

    template<class T>
    inline void print(const T& str) {}

    inline void print(const BSFixedString& str) {
        _DMESSAGE(" %s", str.data);
    }

    inline void print(const SInt32& val) {
        _DMESSAGE(" %u", val);
    }
    inline void print(const char* val) {
        _DMESSAGE(" %s", val);
    }

    inline void print(const Float32& val) {
        _DMESSAGE(" %f", val);
    }


    class array : public collection_base_T< array >
    {
    public:

        enum {
            TypeId = CollectionTypeArray,
        };

        typedef UInt32 Index;

        std::vector<Item> _array;

        void push(const Item& item) {
            mutex_lock g(_mutex);
            _array.push_back(item);
        }

        void clear() override {
            _array.clear();
        }

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

        std::map<std::string, Item> cnt;

        Item& operator[](const std::string& str) {
            mutex_lock g(_mutex);
            return cnt[str];
        }

        void clear() {
            cnt.clear();
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

        std::map<UInt32, Item> cnt;

        Item& operator[](UInt32 str) {
            mutex_lock g(_mutex);
            return cnt[str];
        }

        void clear() {
            cnt.clear();
        }

        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };

   // typedef map<std::string> map;

}










