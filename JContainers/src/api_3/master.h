#pragma once

namespace tes_api_3 {

    template<class T>
    class class_meta : public reflection::class_meta_mixin_t < T > {
    public:
        class_meta() {
            metaInfo.version = (uint32_t)consts::api_version;
        }
    };

    template<class T> T default_value() {
        return static_cast<T>(0);
    };

    template<> collections::form_ref default_value<collections::form_ref>() {
        return collections::form_ref{};
    };
}

#include "api_3/tes_object.h"
#include "api_3/tes_array.h"
#include "api_3/tes_map.h"
#include "api_3/tes_db.h"
#include "api_3/tes_jcontainers.h"
#include "api_3/tes_string.h"
#include "api_3/tes_form_db.h"
#include "api_3/tes_lua.h"

#include "api_3/tests.hpp"
