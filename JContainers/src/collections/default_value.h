#pragma once

#include <type_traits>
#include "forms/form_observer.h"

namespace collections {

    template<class T> inline T default_value() {
        return static_cast<T>(0);
    }

    template<> inline forms::form_ref default_value<forms::form_ref>() {
        return forms::form_ref{};
    }
}
