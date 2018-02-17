#pragma once

#include <type_traits>
#include "forms/form_observer.h"

namespace collections {

    template<class T>
    inline std::enable_if_t<std::is_default_constructible<T>::value, T> default_value() {
        return T{};
    }

    template<class T>
    inline typename std::enable_if_t<std::is_default_constructible<T>::value == false, T> default_value() {
        return static_cast<T>(0);
    }

}
