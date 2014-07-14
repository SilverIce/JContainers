#pragma once

#include <mutex>
#include <atomic>
#include <assert.h>
#include <boost/smart_ptr/intrusive_ptr_jc.hpp>

#include "spinlock.h"

namespace collections {

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

    struct object_base_stack_ref_policy {
        static void retain(object_base * p);
        static void release(object_base * p);
    };

    template <class T>
    using object_stack_ref_template = boost::intrusive_ptr_jc<T, object_base_stack_ref_policy>;

    typedef object_stack_ref_template<object_base> object_stack_ref;

    class object_base
    {
        friend class shared_state;
    public:
        Handle _id;

        std::atomic_int32_t _refCount;
        std::atomic_int32_t _tes_refCount;
        std::atomic_int32_t _stack_refCount;

        CollectionType _type;
    private:
        shared_state *_context;

        void release_counter(std::atomic_int32_t& counter);

    public:

        virtual ~object_base() {}

    public:
        typedef std::lock_guard<spinlock> lock;
        spinlock _mutex;

        explicit object_base(CollectionType type)
            : _refCount(0)      // for now autorelease queue owns object
            , _tes_refCount(0)
            , _stack_refCount(0)
            , _id(HandleNull)
            , _type(type)
            , _context(nullptr)
        {
        }

/*
        object_base()
            : _refCount(0)
            , _tes_refCount(0)
            , _stack_refCount(0)
            , _id(HandleNull)
            , _type(CollectionTypeNone)
            , _context(nullptr)
        {
        }*/

        // for test purpose only!
        // registers (or returns already registered) identifier
        Handle public_id();

        Handle _uid() const {
            return _id;
        }

        Handle uid() {
            return tes_uid();
        }

        // will mark object as publicly exposed
        Handle tes_uid();

        bool is_public() const {
            return _id != HandleNull;
        }

        template<class T> T* as() {
            return (this && T::TypeId == _type) ? static_cast<T*>(this) : nullptr;
        }

        template<class T> const T* as() const {
            return (this && T::TypeId == _type) ? static_cast<const T*>(this) : nullptr;
        }

        object_base * retain() {
            return u_retain();
        }

        object_base * u_retain() {
            ++_refCount;
            return this;
        }

        object_base * tes_retain() {
            ++_tes_refCount;
            return this;
        }

        int32_t refCount() {
            return _refCount + _tes_refCount + _stack_refCount;
        }
        bool noOwners() const {
            return
                _refCount.load() == 0 &&
                _tes_refCount.load() == 0 &&
                _stack_refCount.load() == 0;
        }

/*
        bool noOwners() const {
            return
                _refCount.load(std::memory_order_acquire) == 0 &&
                _tes_refCount.load(std::memory_order_acquire) == 0 &&
                _stack_refCount.load(std::memory_order_acquire) == 0;
        }
*/

        // push object into the queue (which owns it)
        // after some amount of time object will be released
        object_base * prolong_lifetime();

        void release() { release_counter(_refCount); }
        void tes_release() { release_counter(_tes_refCount); }
        void stack_retain() { ++_stack_refCount; }
        void stack_release() { release_counter(_stack_refCount); }


        // releases and then deletes object if no owners
        // true, if object deleted
        bool release_from_queue();

        void set_context(shared_state & ctx) {
            jc_assert(!_context);
            _context = &ctx;
        }

        shared_state& context() const {
            jc_assert(_context);
            return *_context;
        }

        void _registerSelf();

        virtual void u_clear() = 0;
        virtual SInt32 u_count() const = 0;
        virtual void u_onLoaded() {};

        // nillify object cross references to avoid high-level
        // release calls and resulting deadlock
        virtual void u_nullifyObjects() = 0;

        SInt32 s_count() {
            lock g(_mutex);
            return u_count();
        }

        void s_clear() {
            lock g(_mutex);
            u_clear();
        }

        virtual bool is_equal_to(const object_base& other) const {
            return
                _id == other._id &&
                _type == other._type &&
                _refCount == other._refCount &&
                _tes_refCount == other._tes_refCount &&
                _stack_refCount == other._stack_refCount &&
                u_count() == other.u_count();
        }
    };

    inline void object_base_stack_ref_policy::retain(object_base * p) {
        p->stack_retain();
    }

    inline void object_base_stack_ref_policy::release(object_base * p) {
        p->stack_release();
    }

    struct internal_object_lifetime_policy {
        static void retain(object_base * p) {
            p->retain();
        }

        static void release(object_base * p) {
            p->release();
        }
    };

    typedef boost::intrusive_ptr_jc<object_base, internal_object_lifetime_policy> internal_object_ref;


    class object_lock {
        object_base::lock _lock;
    public:
        explicit object_lock(object_base *obj) : _lock(obj->_mutex) {}
        explicit object_lock(object_base &obj) : _lock(obj._mutex) {}

        template<class T, class P>
        explicit object_lock(const boost::intrusive_ptr_jc<T, P>& ref) : _lock(static_cast<object_base&>(*ref)._mutex) {}
        //explicit object_lock(object_base &obj) : _lock(obj._mutex) {}
    };
}
