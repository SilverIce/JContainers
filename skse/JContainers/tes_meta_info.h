#pragma once

#include <string>
#include <vector>
#include <functional>

class VMClassRegistry;

namespace collections {

    namespace tes_binding {

        struct j_type_info {
            std::string tes_type_name;
            std::string tes_arg_name;
        };

        inline j_type_info j_type_info_make(const char * ttn,  const char * tan) {
            j_type_info info = {ttn ? ttn : "", tan ? tan : ""};
            return info;
        }

        typedef j_type_info (*type_info_func)();

        struct FunctionMetaInfo {
            typedef  void (*Registrator)(VMClassRegistry* registry, const char*, const char*);
            typedef  std::vector<type_info_func> (*TypeStrings)();

            Registrator registrator;
            TypeStrings typeStrings;
            const char *args;
            const char *funcName;

            std::function<std::string () > commentFunc;

            FunctionMetaInfo() {
                registrator = nullptr;
                typeStrings = nullptr;
                args = nullptr;
                funcName = nullptr;
            }

            void setComment(const char *comment) {
                commentFunc = [=]() {
                    return std::string(comment ? comment : "");
                };
            }

            template<class T>
            void setComment(T &lambda) {
                commentFunc = lambda;
            }

            void bind(VMClassRegistry *registry, const char *className) {
                registrator(registry, funcName, className);
            }
        };


        struct class_meta_info {
            std::vector<FunctionMetaInfo> methods;
            const char *className;
            const char *extendsClass;
            const char *comment;
            bool initialized;

            class_meta_info() {
                className = "NoClassName";
                extendsClass = NULL;
                comment = nullptr;
                initialized = false;
            }

        };

        template<class T>
        struct class_meta_mixin {

            static class_meta_info& metaInfo() {
                static class_meta_info info;
                if (!info.initialized) {
                    info.initialized = true;

                    T t;

                    T::additionalSetup();
                }

                return info;
            }

            static void additionalSetup() {}

            static void register_me(FunctionMetaInfo *info) {
                metaInfo().methods.push_back(*info);
                //printf("func %s registered\n", info->function_string().c_str());
            }

            static std::string produceTesCode() {
                return code_producer::produceClassCode(metaInfo());
            }

            static void writeSourceToFile() {
                code_producer::produceClassToFile(metaInfo());
            }

            static void bind(VMClassRegistry* registry) {
                assert(metaInfo().className);

                for (auto& itm : metaInfo().methods) {
                    itm.bind(registry, metaInfo().className);
                }
            }
        };
    }
}
