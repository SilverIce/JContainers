#pragma once

#include "skse/PapyrusNativeFunctions.h"
#include "reflection/reflection.h"
#include "skse/string.h"

class BGSListForm;

namespace reflection { namespace binding {

    // traits placeholders
    template<class TesType>
    struct ValueConverter {
        typedef TesType tes_type;

        static const TesType& convert2J(const TesType& val) {
            return val;
        }

        static const TesType& convert2Tes(const TesType& val) {
            return val;
        }
    };

    template<> struct ValueConverter<void> {
        typedef void tes_type;
        template<class T> static void convert2J(T) {}
        template<class T> static void convert2Tes(T) {}
    };

    struct StringConverter {
        typedef skse::string_ref tes_type;

        static const char* convert2J(const skse::string_ref& str) {
            return str.c_str();
        }

        static skse::string_ref convert2Tes(const char * str) {
            return skse::string_ref(str);
        }
    };

    template<class JType> struct GetConv : ValueConverter < JType > {
        //typedef ValueConverter<TesType> Conv;
    };


    template<> struct GetConv<const char*> : StringConverter{};

    //////////////////////////////////////////////////////////////////////////

    template<class T>
    function_parameter typeInfo();

    template<class T> struct j2Str {
        static function_parameter typeInfo() {
            return reflection::binding::typeInfo<T>();
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

    template <typename T, T func> struct proxy;

    template <class R, class Arg0, class Arg1, R (*func)( Arg0, Arg1 ) >
    struct proxy<R (*)(Arg0, Arg1), func>
    {
        typedef R return_type;
        static std::vector<type_info_func> parameter_info() {
            return {
                &j2Str<R>::typeInfo,
                &j2Str<Arg0>::typeInfo,
                &j2Str<Arg1>::typeInfo,
            };
        }


        struct non_void_ret {
            static typename GetConv<R>::tes_type tes_func(
                StaticFunctionTag* tag,
                typename GetConv<Arg0>::tes_type a0,
                typename GetConv<Arg1>::tes_type a1)
            {
                return GetConv<R>::convert2Tes(
                    func(
                        GetConv<Arg0>::convert2J(a0),
                        GetConv<Arg1>::convert2J(a1))
                    );
            }
        };

        struct void_ret {
            static void tes_func(
                StaticFunctionTag* tag,
                typename GetConv<Arg0>::tes_type a0,
                typename GetConv<Arg1>::tes_type a1)
            {
                func(
                    GetConv<Arg0>::convert2J(a0),
                    GetConv<Arg1>::convert2J(a1));
            }
        };

        using tes_func_holder = typename std::conditional<
            std::is_void<R>::value,
            void_ret,
            non_void_ret>::type;

        static void bind(const bind_args& args) {
            args.registry.RegisterFunction(
                new NativeFunction2 <
                        StaticFunctionTag,
                        typename GetConv<R>::tes_type,
                        typename GetConv<Arg0>::tes_type,
                        typename GetConv<Arg1>::tes_type >(args.functionName, args.className, &tes_func_holder::tes_func, &args.registry)
            );
        }
    };

    #define TARGS_NTH(n)            class Arg ## n
    #define PARAM_NTH(n)            Arg##n arg##n
    #define PARAM_NAMELESS_NTH(n)    Arg##n
    #define ARG_NTH(n)              a##n
    #define COMA                    ,

    #define TESTYPE_NTH(n)          typename GetConv<Arg##n>::tes_type
    #define TESPARAM_NTH(n)     TESTYPE_NTH(n) a##n
    #define TESPARAM_CONV_NTH(n)     GetConv<Arg##n>::convert2J(a##n)
    #define TYPESTRING_NTH(n)     &j2Str<Arg##n>::typeInfo


    #define MAKE_PROXY_NTH(N) \
        template <class R DO_##N(TARGS_NTH, COMA, COMA, NOTHING), R (*func)( DO_##N(PARAM_NAMELESS_NTH, NOTHING, COMA, NOTHING) ) >     \
        struct proxy<R (*)(DO_##N(PARAM_NAMELESS_NTH, NOTHING, COMA, NOTHING)), func>     \
            {     \
            typedef R return_type; \
            static std::vector<type_info_func> parameter_info() {\
                return {\
                    &j2Str<R>::typeInfo,\
                    DO_##N(TYPESTRING_NTH, NOTHING, COMA, NOTHING)\
                };\
            }\
            \
            struct non_void_ret {\
                static typename GetConv<R>::tes_type tes_func(\
                    StaticFunctionTag* tag\
                    DO_##N(TESPARAM_NTH, COMA, COMA, NOTHING))\
                {\
                    return GetConv<R>::convert2Tes(\
                        func(\
                            DO_##N(TESPARAM_CONV_NTH, NOTHING, COMA, NOTHING)\
                            )\
                    ); \
                }\
            };\
            struct void_ret {\
                static void tes_func(\
                    StaticFunctionTag* tag\
                    DO_##N(TESPARAM_NTH, COMA, COMA, NOTHING))\
                {\
                    func( DO_##N(TESPARAM_CONV_NTH, NOTHING, COMA, NOTHING) );\
                }\
            };\
            \
            using tes_func_holder = typename std::conditional<\
                std::is_void<R>::value,\
                void_ret,\
                non_void_ret>::type;\
                 \
            static void bind(const bind_args& args) { \
                args.registry.RegisterFunction(\
                    new NativeFunction##N <StaticFunctionTag, typename GetConv<R>::tes_type   \
                    DO_##N(TESTYPE_NTH, COMA, COMA, NOTHING)  >(args.functionName, args.className, &tes_func_holder::tes_func, &args.registry)   \
                );     \
            }     \
        };

#define NOTHING 

#define DO_0(rep, f, m, l)
#define DO_1(rep, f, m_, l)   f rep(1) l
#define DO_2(rep, f, m, l)   DO_1(rep, f, m, NOTHING) m  rep(2) l
#define DO_3(rep, f, m, l)   DO_2(rep, f, m, NOTHING) m  rep(3) l
#define DO_4(rep, f, m, l)   DO_3(rep, f, m, NOTHING) m  rep(4) l
#define DO_5(rep, f, m, l)   DO_4(rep, f, m, NOTHING) m  rep(5) l


    MAKE_PROXY_NTH(0);
    MAKE_PROXY_NTH(1);
    //MAKE_PROXY_NTH(2);
    MAKE_PROXY_NTH(3);
    MAKE_PROXY_NTH(4);

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

        template<class F, class String2, F f>
        inline function_registree(class_info& info, proxy<F, f>, const char* funcname, const char* argument_names, const String2& comment) {
            using namespace ::reflection;

             using Binder = proxy<F, f>;
             static_assert( false == std::is_same<Binder::return_type, const char *>::value, "a trap for 'const char *' return types" );

             function_info metaF;
             metaF.registrator = &Binder::bind;
             metaF.param_list_func = &Binder::parameter_info;
             
             metaF.argument_names = (argument_names) ? (argument_names) : "";
             metaF.setComment(comment);
             metaF.name = funcname;
             metaF.tes_func = &Binder::tes_func_holder::tes_func;
             metaF.c_func = static_cast<c_function>(f);

             info.addFunction(metaF);
        }
    };

#define REGISTERF(func, _funcname, _args, _comment)\
    ::reflection::binding::function_registree CONCAT(_func_registree_, __LINE__){ metaInfo,\
        ::reflection::binding::proxy<decltype(::reflection::binding::msvc_identity(&func)), &func>(), \
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
