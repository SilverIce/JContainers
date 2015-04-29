#include "reflection.h"

#include <map>
#include "skse\PapyrusVM.h"
#include "code_producer.hpp"

namespace reflection {

    void function_info::bind(VMClassRegistry *registry, const char *className) const {
        registrator(registry, className, name.c_str());
        registry->SetFunctionFlags(className, name.c_str(), VMClassRegistry::kFunctionFlag_NoWait);
    }

    const std::map<istring, class_info>& class_database() {

        auto makeDB = []() {
            std::map<istring, class_info> classDB;

            for (auto & item : meta<class_info_creator>::getListConst()) {
                class_info& info = item();

                auto found = classDB.find(info.className());
                if (found != classDB.end()) {
                    found->second.merge_with_extension(info);
                }
                else {
                    classDB[info.className()] = info;
                }
            }

            return classDB;
        };

        static std::map<istring, class_info> classDB = makeDB();
        return classDB;
    }

    const function_info* find_function_of_class(const char * functionName, const char *className) {

        const function_info * fInfo = nullptr;

        auto& db = class_database();
        auto itr = db.find(className);
        if (itr != db.end()) {
            auto& cls = itr->second;
            fInfo = cls.find_function(functionName);
        }

        return fInfo;
    }
}
