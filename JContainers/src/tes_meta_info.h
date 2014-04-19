#pragma once

#include <string>
#include <vector>
#include <functional>

#include "meta.h"

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

            void bind(VMClassRegistry *registry, const char *className) const {
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

            void addFunction(const FunctionMetaInfo& info) {
                methods.push_back(info);
            }

            void bind(VMClassRegistry* registry) const {
                assert(metaInfo.className);

                for (const auto& itm : methods) {
                    itm.bind(registry, className);
                }
            }
        };

        struct class_meta_mixin {

            class_meta_info metaInfo;

            virtual void additionalSetup() {}
        };

        template<class T>
        struct class_meta_mixin_t : class_meta_mixin  {

            static class_meta_info metaInfoFunc() {
                T t;
                t.additionalSetup();

                return t.metaInfo;
            }

            // special support for hack inside REGISTERF macro
            typedef T __Type;
        };

        typedef class_meta_info (*class_meta_info_creator)();

        template<class T>
        inline void foreach_metaInfo_do(T& func) {
            for (auto & item : meta<class_meta_info_creator>::getListConst()) {
                func(item());
            }
        }

#define TES_META_INFO(Class)    \
    static const ::meta<::collections::tes_binding::class_meta_info_creator > g_tesMetaFunc_##Class ( &Class::metaInfoFunc );

    }
}
