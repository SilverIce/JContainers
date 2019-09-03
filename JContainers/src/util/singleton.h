#pragma once

#include <type_traits>
#include <functional>
#include <mutex>
#include <atomic>
#include "boost/assert.hpp"

namespace util {

    template<
        class T,
        // option to not destroy underlying object ->
        // destructors may invoke some un-usable during exit C++ functions, like the ones that provide lock functionality,
        // which may lead to crash at exit
        bool destructorDestroys = true
    >
    struct singleton {
        std::atomic<T*> _si = nullptr;
        std::recursive_mutex _lock;
        std::function<T*()> _ctor;

        singleton() = delete;
        singleton& operator = (const singleton&) = delete;

        template<class Creator>
        explicit singleton(Creator&& ctor) : _ctor(std::forward<Creator>(ctor)) {}

        ~singleton() {
            if (destructorDestroys) {
                delete _si.load(std::memory_order_relaxed);
            }
        }

        T& get() {
            T* tmp = _si.load(std::memory_order_acquire);
            if (tmp == nullptr) {
                std::lock_guard<decltype(_lock)> g(_lock);
                tmp = _si.load(std::memory_order_relaxed);
                if (tmp == nullptr) {
                    tmp = _ctor();
                    BOOST_ASSERT(tmp);
                    _ctor = nullptr;
                    _si.store(tmp, std::memory_order_release);
                }
            }

            return *tmp;
        }
    };


}