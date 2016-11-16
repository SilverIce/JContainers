#pragma once

#include <atomic>
#include <mutex>
#include <type_traits>

namespace util {

    class spinlock
    {
        std::atomic_flag _lock;

    public:

        spinlock() {
            _lock._My_flag = 0;
        }

        void lock() {
            while(_lock.test_and_set(std::memory_order_acquire))  // acquire lock
                ; // spin
        }

        void unlock() {
            _lock.clear(std::memory_order_release);  
        }

        typedef std::lock_guard<spinlock> guard;
    };


    template<
        class Mutex,
        class F,
        class ...Params
    >
    static std::result_of_t<F(Params...)> perform_while_locked(Mutex& mutex, F&& functor, Params&& ...ps) {
            std::lock_guard<Mutex> guard{ mutex };
            return functor(std::forward<Params>(ps) ...);
        }

}
