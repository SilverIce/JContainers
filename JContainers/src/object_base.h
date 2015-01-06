#pragma once

#include <mutex>
#include <atomic>
#include <assert.h>
#include <boost/smart_ptr/intrusive_ptr_jc.hpp>
#include <boost/optional/optional.hpp>
#include "boost/noncopyable.hpp"

#include "spinlock.h"

namespace collections {

    typedef UInt32 HandleT;

    enum Handle : HandleT {
        HandleNull = 0,
    };

    class object_base;
    class object_context;

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

    class object_base// : boost::noncopyable
    {
        object_base(const object_base&);
        object_base& operator=(const object_base&);

        friend class object_context;
    public:
        Handle _id;

        std::atomic_int32_t _refCount;
        std::atomic_int32_t _tes_refCount;
        std::atomic_int32_t _stack_refCount;

        CollectionType _type;
        boost::optional<std::string> _tag;
    private:
        object_context *_context;

        void release_counter(std::atomic_int32_t& counter);

    public:

        virtual ~object_base() {}

    public:
        typedef std::lock_guard<spinlock> lock;
        mutable spinlock _mutex;

        explicit object_base(CollectionType type)
            : _refCount(0)      // for now autorelease queue owns object
            , _tes_refCount(0)
            , _stack_refCount(0)
            , _id(HandleNull)
            , _type(type)
            , _context(nullptr)
        {
        }

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
            return const_cast<T*>(const_cast<const object_base*>(this)->as<T>());
        }

        template<class T> const T* as() const {
            return (this && T::TypeId == _type) ? static_cast<const T*>(this) : nullptr;
        }

        template<class T> T& as_link() {
            return const_cast<T&>(const_cast<const object_base*>(this)->as_link<T>());
        }

        template<class T> const T& as_link() const {
            auto obj = this->as<T>();
            assert(obj);
            return *obj;
        }

        object_base * retain() {
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
                _refCount.load() <= 0 &&
                _tes_refCount.load() <= 0 &&
                _stack_refCount.load() <= 0;
        }

        bool u_is_user_retains() const {
            return _tes_refCount.load(std::memory_order_relaxed) > 0;
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

        void release();
        void tes_release();
        void stack_retain() { ++_stack_refCount; }
        void stack_release();

        // releases and then deletes object if no owners
        // true, if object deleted
        bool _final_release();
        void _delete_self();

        void set_context(object_context & ctx) {
            jc_assert(!_context);
            _context = &ctx;
        }

        object_context& context() const {
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

        // empty string is also null tag
        void set_tag(const char *tag) {
            lock g(_mutex);
            if (tag && *tag) {
                _tag = tag;
            } else {
                _tag = boost::none;
           }
        }

        bool has_equal_tag(const char *tag) const {
            lock l(_mutex);
            return _tag && tag ? _strcmpi(_tag.get().c_str(), tag) == 0 : false;
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

        virtual void u_visit_referenced_objects(const std::function<void(object_base&)>& visitor) {}
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
        explicit object_lock(const object_base *obj) : _lock(obj->_mutex) {}
        explicit object_lock(const object_base &obj) : _lock(obj._mutex) {}

        template<class T, class P>
        explicit object_lock(const boost::intrusive_ptr_jc<T, P>& ref) : _lock(static_cast<const object_base&>(*ref)._mutex) {}
    };
}
