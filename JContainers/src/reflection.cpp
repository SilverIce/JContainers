#include "reflection.h"

#include "code_producer.hpp"
#include "skse\PapyrusVM.h"

namespace reflection {

    void function_info::bind(VMClassRegistry *registry, const char *className) const {
        registrator(registry, name, className);
        registry->SetFunctionFlags(className, name, VMClassRegistry::kFunctionFlag_NoWait);
    }
    
    extern "C" {

        __declspec(dllexport) void produceCode() {

            using namespace collections;

            foreach_metaInfo_do([](const class_info& info) {
                code_producer::produceClassToFile(info);
            });
        }
};

}
