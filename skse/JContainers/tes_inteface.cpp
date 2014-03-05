
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#include <set>

#include "cJSON.h"
#include "gtest.h"

#include "tes_error_code.h"

#include "collections.h"
#include "autorelease_queue.h"
#include "shared_state.h"
#include "json_handling.h"

#include "tes_binding.h"

#include "tes_object.h"
#include "tes_array.h"
#include "tes_map.h"
#include "tes_db.h"
#include "tes_jcontainers.h"

#include "collections.tests.hpp"

namespace collections {

}

bool registerAllFunctions(VMClassRegistry *registry) {

    using namespace collections;

    tes_binding::foreach_metaInfo_do([=](tes_binding::class_meta_info& info) {
        info.bind(registry);
    });
/*
    collections::tes_array::registerFuncs(registry);

    collections::tes_map::registerFuncs(registry);
    collections::tes_form_map::registerFuncs(registry);

    collections::tes_object::registerFuncs(registry);

    collections::tes_db::registerFuncs(registry);

    collections::tes_jcontainers::bind(registry);
*/

    return true;
}

extern "C" {

    __declspec(dllexport) void produceCode() {

        using namespace collections;

        tes_binding::foreach_metaInfo_do([](tes_binding::class_meta_info& info) {
            CodeProducer::produceClassToFile(info);
        });
        /*
/*
        collections::tes_array::writeSourceToFile();
        collections::tes_map::writeSourceToFile();
        collections::tes_form_map::writeSourceToFile();
        collections::tes_object::writeSourceToFile();
        collections::tes_db::writeSourceToFile();
        collections::tes_jcontainers::writeSourceToFile();*/
    }
};
