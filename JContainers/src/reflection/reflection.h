#pragma once

#include <string>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <stdint.h>

#include "meta.h"
#include "util/istring.h"

class VMClassRegistry;

namespace reflection {

    using istring = util::istring;

    struct function_parameter {
        std::string tes_type_name;
        std::string tes_arg_name;
    };

    inline function_parameter function_parameter_make(const char * type_name, const char * arg_name) {
        return { type_name ? type_name : "", arg_name ? arg_name : "" };
    }

    typedef function_parameter (*type_info_func)();

    typedef void* tes_api_function;
    typedef void* c_function;

    struct bind_args {
        VMClassRegistry& registry;
        const char* className;
        const char* functionName;
    };

    struct function_info {
        typedef std::string (*comment_generator)();
        typedef  void(*tes_function_binder)(const bind_args& args);
        typedef  std::vector<type_info_func> (*parameter_list_creator)();

        tes_function_binder registrator = nullptr;
        parameter_list_creator param_list_func = nullptr;
        tes_api_function tes_func = nullptr; // the function which gets exported into Papyrus (+1 argument, papyrus args only)
        c_function c_func = nullptr; // original function
        istring argument_names;
        istring name;

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

        void bind(VMClassRegistry& registry, const istring& className) const;
    };

    struct papyrus_text_block {
        typedef std::string(*text_generator)();

        papyrus_text_block(const char * t) : _text(t) {}
        papyrus_text_block(text_generator t) : _text_generator_func(t) {}

        const char * _text = nullptr;
        text_generator _text_generator_func = nullptr;

        std::string get_text() const {
            return _text ? _text : _text_generator_func();
        }
    };

    struct class_info {
        std::vector<function_info > methods;
        std::vector<papyrus_text_block > text_blocks;
        istring _className;
        istring extendsClass;
        std::string comment;
        uint32_t version = 0;

        class_info() {
            _className = "NoClassName";
        }

        bool initialized() const {
            return !_className.empty();
        }

        istring className() const {
#   if 0
            return _className + '_' + (char)((uint32_t)'0' + version);
#   else
            return _className;
#   endif
        }

        void add_text_block(const papyrus_text_block& tb) {
            text_blocks.push_back(tb);
        }

        const function_info * find_function(const char* func_name) const {
            auto itr = std::find_if(methods.begin(), methods.end(), [&](const function_info& fi) { return func_name == fi.name; });
            return itr != methods.end() ? &*itr : nullptr;
        }

        void addFunction(const function_info& info) {
            assert(find_function(info.name.c_str()) == nullptr);
            methods.push_back(info);
        }

        void bind(VMClassRegistry& registry) const {
            assert(initialized());

            auto clsName = className();
            for (const auto& itm : methods) {
                itm.bind(registry, clsName);
            }
        }

        void merge_with_extension(const class_info& extension) {
            assert(initialized());
            assert(className() == extension.className());
            assert(extendsClass == extension.extendsClass);

            for (const auto& itm : extension.methods) {
                addFunction(itm);
            }
        }
    };

    // Produces script files using meta class information
    namespace code_producer {

        std::string produceClassCode(const class_info& self);
        void produceClassToFile(const class_info& self, const std::string& directoryPath);

    }

    namespace _detail {

        struct class_meta_mixin {
            class_info metaInfo;
            virtual void additionalSetup() {}
        };

        template<class Derived>
        struct class_meta_mixin_t : class_meta_mixin  {

            static class_info metaInfoFunc() {
                Derived t;
                t.additionalSetup();
                return t.metaInfo;
            }
        };
    }

    template <class Derived>
    using class_meta_mixin_t = _detail::class_meta_mixin_t<Derived>;

    const std::map<istring, class_info>& class_registry();
    const function_info* find_function_of_class(const char * functionName, const char *className);

    template<class T>
    inline void foreach_metaInfo_do(T&& func) {
        for (auto & pair : class_registry()) {
            func(pair.second);
        }
    }

    // A function that creates meta info of Papyrus class. One function per class
    typedef class_info (*class_info_creator)();

#   define TES_META_INFO(Class)    \
        static const ::meta<::reflection::class_info_creator > g_tesMetaFunc_##Class ( &Class::metaInfoFunc );

}
