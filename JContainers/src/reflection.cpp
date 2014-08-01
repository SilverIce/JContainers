#include "reflection.h"
#include "skse\PapyrusVM.h"

#include "code_producer.hpp"

namespace reflection {

    void function_info::bind(VMClassRegistry *registry, const char *className) const {
        registrator(registry, className, name);
        registry->SetFunctionFlags(className, name, VMClassRegistry::kFunctionFlag_NoWait);
    }

    const std::map<std::string, class_info, string_icomparison>& class_database() {

        auto makeDB = []() {
            std::map<std::string, class_info, string_icomparison> classDB;

            for (auto & item : meta<class_info_creator>::getListConst()) {
                class_info& info = item();

                auto found = classDB.find(info.className);
                if (found != classDB.end()) {
                    found->second.merge_with_extension(info);
                }
                else {
                    classDB[info.className] = info;
                }
            }

            return classDB;
        };

        static std::map<std::string, class_info, string_icomparison> classDB = makeDB();
        return classDB;
    }
    
    extern "C" {

        __declspec(dllexport) void produceCode() {

            foreach_metaInfo_do([](const class_info& info) {
                code_producer::produceClassToFile(info);
            });
        }
    };

}
