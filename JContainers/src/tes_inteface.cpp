
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#include <set>
#include <thread>

#include <boost/filesystem.hpp>

#include "cJSON/cJSON.h"
#include "gtest.h"
#include "plugin_info.h"

#include "tes_error_code.h"

#include "collections.h"
#include "shared_state.h"
#include "json_handling.h"

#include "tes_binding.h"

#include "tes_object.h"
#include "tes_array.h"
#include "tes_map.h"
#include "tes_db.h"
#include "tes_jcontainers.h"
#include "tes_string.h"
#include "tes_form_db.h"

#include "collections.tests.hpp"

namespace collections {

}

bool registerAllFunctions(VMClassRegistry *registry) {

    _MESSAGE("registering functions");

    using namespace collections;

    tes_binding::foreach_metaInfo_do([=](const tes_binding::class_meta_info& info) {
        info.bind(registry);
    });

    return true;
}

extern "C" {

    __declspec(dllexport) void produceCode() {

        using namespace collections;

        tes_binding::foreach_metaInfo_do([](const tes_binding::class_meta_info& info) {
            code_producer::produceClassToFile(info);
        });
    }
};
