#pragma once

#include "spinlock.h"
#include <mutex>

namespace collections {
    typedef spinlock object_mutex;

    typedef std::lock_guard<object_mutex> mutex_lock;

    typedef UInt32 HandleT;

    enum Handle : HandleT {
        HandleNull = 0,
    };

    class object_base;
    class shared_state;

    enum CollectionType
    {
        CollectionTypeNone = 0,
        CollectionTypeArray,
        CollectionTypeMap,
        CollectionTypeFormMap,
    };

    class object_base
    {
        friend class shared_state;

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
        shared_state *_context;

        explicit object_base(CollectionType type)
            : _refCount(1)
            , _tes_refCount(0)
            , id(HandleNull)
            , _type(type)
            , _context(nullptr)
        {
        }

        object_base()
            : _refCount(0)
            , _tes_refCount(0)
            , _id(0)
            , _type(CollectionTypeNone)
            , _context(nullptr)
        {
        }

        int32_t refCount() {
            mutex_lock g(_mutex);
            return _refCount + _tes_refCount;
        }

        bool registered() const {
            return _id != 0;
        }

        template<class T> T* as() {
            return (this && T::TypeId == _type) ? static_cast<T*>(this) : nullptr;
        }

        object_base * retain() {
            mutex_lock g(_mutex);
            return u_retain();
        }

        object_base * u_retain() {
            ++_refCount;
            return this;
        }

        object_base * tes_retain() {
            mutex_lock g(_mutex);
            ++_tes_refCount;
            return this;
        }

        object_base * autorelease();

        // decreases internal ref counter - _refCount OR deletes if summ refCount is 0
        // if old refCountSumm is 1 - then release, if 0 - delete
        // true, if object deleted
        bool _deleteOrRelease(class autorelease_queue*);

        void release();
        void tes_release();
        void _addToDeleteQueue();
        void _registerSelf();

        virtual void u_clear() = 0;
        virtual SInt32 u_count() = 0;

        virtual void u_onLoaded() {};

        // nillify object cross references to avoid high-level
        // release calls and resulting deadlock
        virtual void u_nullifyObjects() = 0;

        SInt32 s_count() {
            mutex_lock g(_mutex);
            return u_count();
        }

        void s_clear() {
            mutex_lock g(_mutex);
            u_clear();
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & _refCount;
            ar & _tes_refCount;

            ar & _type;
            ar & _id;
        }
    };

    class object_lock {
        mutex_lock _lock;
    public:
        explicit object_lock(object_base *obj) : _lock(obj->_mutex) {}
        explicit object_lock(object_base &obj) : _lock(obj._mutex) {}
    };
}
