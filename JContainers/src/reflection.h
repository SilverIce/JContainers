#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <assert.h>

#include "meta.h"

class VMClassRegistry;

namespace reflection {

    struct function_parameter {
        std::string tes_type_name;
        std::string tes_arg_name;
    };

    inline function_parameter function_parameter_make(const char * type_name, const char * arg_name) {
        return { type_name ? type_name : "", arg_name ? arg_name : "" };
    }

    typedef function_parameter (*type_info_func)();

    struct function_info {
        typedef  void (*tes_function_binder)(VMClassRegistry* registry, const char* className, const char* funcName);
        typedef  std::vector<type_info_func> (*parameter_list_creator)();

        tes_function_binder registrator;
        parameter_list_creator param_list_func;
        const char *argument_names;
        const char *name;

        std::function<std::string () > commentFunc;

        function_info() {
            registrator = nullptr;
            param_list_func = nullptr;
            argument_names = nullptr;
            name = nullptr;
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

        void bind(VMClassRegistry *registry, const char *className) const;
    };

    struct class_info {

        struct function_info_comparison {
            bool operator() (const function_info& l, const function_info& r) const { return _strcmpi(l.name, r.name) < 0; }
        };

        std::set<function_info, function_info_comparison > methods;
        std::string className;
        std::string extendsClass;
        std::string comment;

        class_info() {
            className = "NoClassName";
        }

        void addFunction(const function_info& info) {
            assert(methods.find(info) == methods.end());
            methods.insert(info);
        }

        void bind(VMClassRegistry* registry) const {
            assert(!className.empty());

            for (const auto& itm : methods) {
                itm.bind(registry, className.c_str());
            }
        }

        void merge_with_extension(const class_info& extension) {
            assert(className == extension.className);
            assert(extendsClass == extension.extendsClass);

            for (const auto& itm : extension.methods) {
                addFunction(itm);
            }
        }
    };

    // Produces script files using meta class information
    namespace code_producer {

        std::string produceClassCode(const class_info& self);
        void produceClassToFile(const class_info& self);

    }

    namespace _detail {

        struct class_meta_mixin {
            class_info metaInfo;
            virtual void additionalSetup() {}
        };

        template<class T>
        struct class_meta_mixin_t : class_meta_mixin  {

            static class_info metaInfoFunc() {
                T t;
                t.additionalSetup();
                return t.metaInfo;
            }
            // special support for hack inside REGISTERF macro
            typedef T __Type;
        };
    }
    template <class T>
    using class_meta_mixin_t = _detail::class_meta_mixin_t<T>;

    typedef class_info (*class_info_creator)();

    template<class T>
    inline void foreach_metaInfo_do(T& func) {

        std::map<std::string, class_info> classDB;

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

        for (auto & pair : classDB) {
            func(pair.second);
        }
    }

#   define TES_META_INFO(Class)    \
        static const ::meta<::reflection::class_info_creator > g_tesMetaFunc_##Class ( &Class::metaInfoFunc );

}
