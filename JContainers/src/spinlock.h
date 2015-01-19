#pragma once

#include <atomic>
#include <mutex>

namespace collections {

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
}
