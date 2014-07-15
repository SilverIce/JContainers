#pragma once

#include <string>
#include <vector>
#include <assert.h>
#include <algorithm>

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
        typedef std::string (*comment_generator)();
        typedef  void (*tes_function_binder)(VMClassRegistry* registry, const char* className, const char* funcName);
        typedef  std::vector<type_info_func> (*parameter_list_creator)();

        tes_function_binder registrator = nullptr;
        parameter_list_creator param_list_func = nullptr;
        const char *argument_names = nullptr;
        const char *name = nullptr;

        comment_generator _comment_func = nullptr;
        const char *_comment_str = nullptr;

        std::string comment() const {
            if (_comment_func) {
                return _comment_func();
            }

            return _comment_str ? _comment_str : "";
        }

        void setComment(comment_generator func) {
            _comment_func = func;
        }

        void setComment(nullptr_t) {
            _comment_func = nullptr;
            _comment_str = nullptr;
        }

        void setComment(const char * comment) {
            _comment_str = comment;
        }

        void bind(VMClassRegistry *registry, const char *className) const;
    };

    struct class_info {

        std::vector<function_info > methods;
        std::string className;
        std::string extendsClass;
        std::string comment;

        class_info() {
            className = "NoClassName";
        }

        function_info * find_function(const char* func_name) {
            auto itr = std::find_if(methods.begin(), methods.end(), [&](const function_info& fi) { return _strcmpi(func_name, fi.name) == 0; });
            return itr != methods.end() ? &*itr : nullptr;
        }

        void addFunction(const function_info& info) {
            assert(find_function(info.name) == nullptr);
            methods.push_back(info);
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
