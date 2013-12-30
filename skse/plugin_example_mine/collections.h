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

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/split_member.hpp>

#include <mutex>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/thread/lock_guard.hpp>

#include "id_generator.h"

class VMClassRegistry;
struct StaticFunctionTag;
class T_Base;

namespace collections {

    typedef std::lock_guard<std::mutex> mutex_lock;
    using std::mutex;

    typedef boost::shared_mutex bshared_mutex;
    typedef boost::lock_guard<bshared_mutex> write_lock;
    typedef boost::shared_lock_guard<bshared_mutex> read_lock;

    typedef UInt32 HandleT;

    enum Handle : HandleT {
        HandleNull = 0,
    };

    class collection_base;

    enum CollectionType
    {
        CollectionTypeNone = 0,
        CollectionTypeArray,
        CollectionTypeMap,
    };

    class collection_registry
    {
        std::map<HandleT, collection_base *> _map;
        id_generator<HandleT> _idGen;
        bshared_mutex& _mutex;

        collection_registry(const collection_registry& );
        collection_registry& operator = (const collection_registry& );

    public:

        collection_registry(bshared_mutex &mt) : _mutex(mt) {}

        static Handle registerObject(collection_base *collection);

        static void removeObject(HandleT hdl) {
            if (!hdl) {
                return;
            }
            
            collection_registry& me = instance();
            write_lock g(me._mutex);
            me._map.erase(hdl);
            me._idGen.reuseId(hdl);
        }

        static collection_base *getObject(HandleT hdl) {
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
        void serialize(Archive & ar, const unsigned int version) {
            ar.register_type(static_cast<array *>(NULL));
            ar.register_type(static_cast<map *>(NULL));

            ar & _map;
            ar & _idGen;
        }
    };

    class collection_base
    {
        int32_t _refCount;

    public:

        virtual ~collection_base() {}

    public:
        std::mutex _mutex;
        union {
            Handle id;
            UInt32 _id;
        };

        CollectionType _type;

        explicit collection_base(CollectionType type)
            : _refCount(1)
            , id(HandleNull)
            , _type(type)
        {
        }

        collection_base()
            : _refCount(0)
            , _id(0)
            , _type(CollectionTypeNone)
        {
        }

        int refCount() const {
            return _refCount;
        }

        bool registered() const {
            return _id != 0;
        }

        template<class T> T* as() {
            return T::TypeId == _type ? static_cast<T*>(this) : nullptr;
        }

        collection_base * retain() {
            mutex_lock g(_mutex);
            ++_refCount;
            return this;
        }

        collection_base * autorelease();

        void release() {
            _mutex.lock();
            --_refCount;
            if (_refCount == 0) {
                collection_registry::removeObject(id);
                _mutex.unlock();
                delete this;
            } else {
                _mutex.unlock();
            }
        }

        virtual void clear() = 0;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & _refCount;
            ar & _type;
            ar & _id;
        }
    };

    Handle collection_registry::registerObject(collection_base *collection) {
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

    void collection_registry::_clear() {
        /*  Not good, but working solution.

            issue: deadlock during loading savegame - e.g. cleaning current state.

            due to: collection release -> dealloc -> collection_registry::removeObject
                 introduces deadlock ( registry locked during _clear call)

            solution: +1 refCount to all objects & clear & delete them

            all we need is just free the memory but this will require track allocated collection & stl memory blocks 
        */
        for (auto pair : _map) {
            pair.second->retain(); // to guarant that removeObject will not be called during clear method call
        }
        for (auto pair : _map) {
            pair.second->clear(); // to force ~Item() calls while all collections alive (~Item() may release collection)
        }
        for (auto pair : _map) {
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
    class collection_base_T : public collection_base
    {
    protected:

        collection_base_T() : collection_base((CollectionType)T::TypeId) {}

    public:

        static T* create() {
            auto obj = new T();
            collection_registry::registerObject(obj);
            return obj;
        }

        static T* object() {
            return (T *)create()->autorelease();
        }
    };

    enum ItemType : unsigned char
    {
        ItemTypeNone = 0,
        ItemTypeInt32 = 1,
        ItemTypeFloat32 = 2,
        ItemTypeCString = 3,
        ItemTypeObject = 4,
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
            SInt32 _intVal;
            Float32 _floatVal;
            StringMem *_stringVal;
            collection_base *_object;
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

        explicit Item(const char * val) : _stringVal(NULL), _type(ItemTypeNone) {
            setStringVal(val);
        }

        explicit Item(const BSFixedString& str) : _stringVal(NULL), _type(ItemTypeNone) {
            setStringVal(str.data);
        }

        explicit Item(collection_base *collection) : _stringVal(NULL), _type(ItemTypeNone) {
            setObjectVal(collection);
        }

        explicit Item(Handle hdl) : _stringVal(NULL), _type(ItemTypeNone) {
            auto obj = collection_registry::getObject(hdl);
            if (obj)
                setObjectVal(obj);
        }

        bool operator == (const Item& other) const {
            return _type == other._type && _object == other._object;
        }

        bool operator != (const Item& other) const {
            return !(*this == other);
        }

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const {
            ar.register_type(static_cast<array *>(NULL));
            ar.register_type(static_cast<map *>(NULL));

            ar & _type;
            switch (_type)
            {
            case ItemTypeNone:
                break;
            case ItemTypeInt32:
                ar & _intVal;
                break;
            case ItemTypeFloat32:
                ar & _floatVal;
                break;
            case ItemTypeCString: 
                ar & std::string(strValue() ? _stringVal->string : "");
                break;
            case ItemTypeObject:
                ar & _object;
                break;
            default:
                break;
            }
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            ar.register_type(static_cast<array *>(NULL));
            ar.register_type(static_cast<map *>(NULL));

            ar & _type;
            switch (_type)
            {
            case ItemTypeNone:
                break;
            case ItemTypeInt32:
                ar & _intVal;
                break;
            case ItemTypeFloat32:
                ar & _floatVal;
                break;
            case ItemTypeCString:
            {
                std::string string;
                ar & string;

                if (!string.empty()) {
                    _stringVal = StringMem::allocWithString(string.c_str());
                }
                break;
            }
            case ItemTypeObject:
                ar & _object;
                break;
            default:
                break;
            }
        }

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

        void setObjectVal(collection_base *obj) {
            if (objValue() == obj)
                return;

            _replaceObject(obj);
        }

        bool _freeObject() {
            collection_base *prev = objValue();
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

        void _replaceObject(collection_base *obj) {
            if (!_freeObject())
                _freeString();

            _type = ItemTypeObject;
            _object = obj;

            if (obj)
                obj->retain();
        }

        void setType(ItemType iType) {
            if (iType != ItemTypeCString) {
                _freeString();
            }

            _type = iType;
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
            } else if (other.objValue()){
                setObjectVal(other.objValue());
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

        bool isNull() const {
            return _type == ItemTypeNone;
        }

        collection_base *objValue() const {
            return (_type == ItemTypeObject) ? _object : nullptr;
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
        auto obj = objValue();
        return obj ? obj->id : HandleNull;
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

   
        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            mutex_lock g(_mutex);
            ar & boost::serialization::base_object<collection_base>(*this);
            ar & _array;
        }


    };

    class map : public collection_base_T< map >
    {
    public:

        enum 
        {
            TypeId = CollectionTypeMap,
        };

        typedef const char * Key;

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
        void serialize(Archive & ar, const unsigned int version) {
            mutex_lock g(_mutex);
            ar & boost::serialization::base_object<collection_base>(*this);
            ar & cnt;
        }
    };

}


#include "collections.from_json.h"
#include "autorelease_queue.h"
#include "serialization.h"
#include "collections.tesregistration.hpp"

#include "collections.tests.hpp"







