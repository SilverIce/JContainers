#pragma once

#include "skse/PapyrusNativeFunctions.h"
#include "skse/string.h"
#include "reflection/reflection.h"

class BGSListForm;

namespace reflection { namespace binding {

    // traits placeholders
    template<class JType>
    struct IdentityConverter {
        typedef JType tes_type;

        static const tes_type& convert2J(const tes_type& val) {
            return val;
        }

        static const tes_type& convert2Tes(const tes_type& val) {
            return val;
        }
    };

    template<class JType, class TesType>
    struct StaticCastValueConverter {
        typedef TesType tes_type;

        static JType convert2J(const TesType& val) {
            return static_cast<JType>(val);
        }

        static TesType convert2Tes(const JType& val) {
            return static_cast<TesType>(val);
        }
    };

    struct StringConverter {
        typedef skse::string_ref tes_type;

        static const char* convert2J(const skse::string_ref& str) {
            return str.c_str();
        }

        template<class AnyString>
        static skse::string_ref convert2Tes(const AnyString& str) {
            return skse::string_ref(str);
        }
    };

    template<class JType> struct GetConv : IdentityConverter < JType > {
        //typedef ValueConverter<TesType> Conv;
    };

    template<> struct GetConv<void> {
        using tes_type = void;
    };

    template<> struct GetConv<const char*> : StringConverter{};
    template<> struct GetConv<std::string> : StringConverter{};

    template<> struct GetConv<int32_t> : StaticCastValueConverter<int32_t, SInt32>{};

    //////////////////////////////////////////////////////////////////////////

    template<class T>
    function_parameter type_info();

    template<class T> struct j2Str {
        static function_parameter typeInfo() {
            return reflection::binding::type_info<T>();
        }
    };

    template<class T> struct j2Str < VMArray<T> > {
        static function_parameter typeInfo() {
            std::string str(j2Str<T>::typeInfo().tes_type_name);
            str += "[]";
            function_parameter info = { str, "values" };
            return info;
        }
    };

    template<class T> struct j2Str < VMResultArray<T> > : j2Str < VMArray<T> > {};

    //////////////////////////////////////////////////////////////////////////

    template <typename T> struct proxy;


    template<size_t ParamCnt>
    struct native_function_selector;

#define MAKE_ME_HAPPY(N)\
    template<> struct native_function_selector<N> {\
        template<class... Params> using function = ::NativeFunction ## N <::StaticFunctionTag, Params...>;\
    };

    MAKE_ME_HAPPY(0);
    MAKE_ME_HAPPY(1);
    MAKE_ME_HAPPY(2);
    MAKE_ME_HAPPY(3);
    MAKE_ME_HAPPY(4);
    MAKE_ME_HAPPY(5);
    MAKE_ME_HAPPY(6);

#undef  MAKE_ME_HAPPY

    template <class R, class... Params>
    struct proxy<R (*)(Params ...)>
    {
        static std::vector<type_info_func> parameter_info() {
            return {
                &j2Str<R>::typeInfo,
                &j2Str<Params>::typeInfo ...
            };
        }

        using return_type = R;

        template< R(*func)(Params ...) >
        struct magick {

            using base = proxy;

            static auto func_ptr() -> decltype(func) {
                return func;
            }

            struct non_void_ret {
                static typename GetConv<R>::tes_type tes_func(
                    StaticFunctionTag* tag,
                    typename GetConv<Params>::tes_type ... params)
                {
                    return GetConv<R>::convert2Tes(
                        func(
                            GetConv<Params>::convert2J(params) ...
                        )
                    );
                }
            };

            struct void_ret {
                static void tes_func(
                    StaticFunctionTag* tag,
                    typename GetConv<Params>::tes_type ... params)
                {
                    func(GetConv<Params>::convert2J(params) ...);
                }
            };

            using tes_func_holder = typename std::conditional<
                std::is_void<R>::value,
                typename void_ret,
                typename non_void_ret>::type;

            static void bind(const bind_args& args) {
                args.registry.RegisterFunction
                (
                    new typename native_function_selector<sizeof...(Params)>::function<
                        typename GetConv<R>::tes_type, typename GetConv<Params>::tes_type ...>
                        (
                            args.functionName,
                            args.className,
                            &tes_func_holder::tes_func,
                            &args.registry
                        )
                );
            }
        };


    };

#define CONCAT(x, y) CONCAT1 (x, y)
#define CONCAT1(x, y) x##y

    // MSVC2012 bug workaround
    template <typename T> T msvc_identity(T);

    struct name_setter {
        explicit name_setter(class_info& info, const char* className) {
            info._className = className;
        }
    };

#define REGISTER_TES_NAME(ScriptTesName)  \
    ::reflection::binding::name_setter _name_setter{ metaInfo, ScriptTesName };

    struct function_registree {

        template<class Binder, class String2>
        inline function_registree(class_info& info,
            Binder,
            const char* funcname, const char* argument_names, const String2& comment)
        {
            using namespace ::reflection;

            static_assert( false == std::is_same<Binder::base::return_type, const char *>::value, "a trap for 'const char *' return types" );

            function_info metaF;
            metaF.registrator = &Binder::bind;
            metaF.param_list_func = &Binder::base::parameter_info;
             
            metaF.argument_names = (argument_names) ? (argument_names) : "";
            metaF.setComment(comment);
            metaF.name = funcname;
            metaF.tes_func = &Binder::tes_func_holder::tes_func;
            metaF.c_func = static_cast<c_function>(Binder::func_ptr());

            info.addFunction(metaF);
        }
    };

#define REGISTERF(func, _funcname, _args, _comment)\
    ::reflection::binding::function_registree CONCAT(_func_registree_, __LINE__){ metaInfo,\
        ::reflection::binding::proxy<decltype(::reflection::binding::msvc_identity(&func))>::magick<&func>(), \
        _funcname, _args, _comment };

#define REGISTERF2(func, args, comment)     REGISTERF(func, #func, args, comment)

    struct papyrus_textblock_setter {
        explicit papyrus_textblock_setter(class_info& info, const papyrus_text_block& text) {
            info.add_text_block(text);
        }
    };

#define REGISTER_TEXT(text) \
    ::reflection::binding::papyrus_textblock_setter CONCAT(_textblock_setter_, __LINE__){ metaInfo, ::reflection::papyrus_text_block(text) };


}
}
