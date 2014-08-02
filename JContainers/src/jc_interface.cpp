#include "jc_interface.h"
#include "reflection.h"

namespace jc { namespace {

    const reflection_interface refl = {
        reflection_interface::version,
        reflection::find_tes_function_of_class,
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
