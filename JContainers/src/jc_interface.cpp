#include "jc_interface.h"
#include "reflection/reflection.h"

namespace jc { namespace {

    const reflection_interface refl = {
        reflection_interface::version,
        [](const char * functionName, const char *className) -> void* {
            auto fi = reflection::find_function_of_class(functionName, className);
            return fi ? fi->tes_func : nullptr;
        }
    };

    const void * query_interface(uint32_t id) {
        switch (id) {
        case reflection_interface::type_id:
            return &refl;
        }
        return nullptr;
    }

}

    root_interface root = {
        root_interface::version,
        query_interface,
    };
}
