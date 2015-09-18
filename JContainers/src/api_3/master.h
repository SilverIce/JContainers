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

    template<> collections::weak_form_id default_value<collections::weak_form_id>() {
        return collections::weak_form_id{};
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
