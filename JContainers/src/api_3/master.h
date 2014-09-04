#pragma once

namespace tes_api_3 {

    template<class T>
    class class_meta : public reflection::class_meta_mixin_t < T > {
    public:
        class_meta() {
            metaInfo.version = kJAPIVersion;
        }
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

#include "api_3/collections.tests.hpp"
