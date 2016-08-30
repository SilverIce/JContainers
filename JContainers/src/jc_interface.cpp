#include "jc_interface.h"

#include "util/istring.h"
#include "reflection/reflection.h"
#include "domains/domain_master.h"

namespace jc { namespace {

    const reflection_interface refl = {
        reflection_interface::version,
        [](const char * functionName, const char *className) -> void* {
            auto fi = reflection::find_function_of_class(functionName, className);
            return fi ? fi->tes_func : nullptr;
        }
    };

    const domain_interface dom = {
        domain_interface::version,
        []() -> void* {
            return &domain_master::master::instance().get_default_domain();
        },
        [](const char *domain_name) -> void* {
            return domain_name
                ? domain_master::master::instance().get_domain_with_name(util::istring{ domain_name })
                : nullptr;
        }
    };


    const void * query_interface(uint32_t id) {
        switch (id) {
        case reflection_interface::type_id:
            return &refl;
        case domain_interface::type_id:
            return &dom;
        }
        return nullptr;
    }

}

    root_interface root = {
        root_interface::version,
        query_interface,
    };
}
