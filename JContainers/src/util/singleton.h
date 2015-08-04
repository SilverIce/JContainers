#pragma once

#include <type_traits>
#include <functional>
#include "boost/assert.hpp"

#include "util/spinlock.h"

namespace util {

    template<class T>
    struct singleton {
        std::atomic<T*> _si = nullptr;
        spinlock _lock;
        std::function<T*()> _ctor;

        singleton() = delete;
        singleton& operator = (const singleton&) = delete;

        template<class Creator>
        explicit singleton(Creator&& ctor) : _ctor(std::forward<Creator>(ctor)) {}

        ~singleton() {
            delete _si.load(std::memory_order_relaxed);
        }

        T& get() {
            T* tmp = _si.load(std::memory_order_acquire);
            if (tmp == nullptr) {
                spinlock::guard g(_lock);
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