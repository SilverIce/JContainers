#pragma once

#include "dyn_form_watcher.h"

namespace collections {

    template<class T> inline T default_value() {
        return static_cast<T>(0);
    };

    template<> inline form_ref default_value<form_ref>() {
        return form_ref{};
    };
}
