#pragma once

#include "skse/PapyrusNativeFunctions.h"
#include "reflection.h"

namespace reflection { namespace binding {

    // traits placeholders
    template<class TesType>
    struct ValueConverter {
        typedef TesType tes_type;

        static TesType convert2J(const TesType& val) {
            return val;
        }

        static TesType convert2Tes(const TesType& val) {
            return val;
        }
    };

    struct StringConverter {
        typedef BSFixedString tes_type;

        static const char* convert2J(const BSFixedString& str) {
            return str.data;
        }

        static BSFixedString convert2Tes(const char * str) {
            return BSFixedString(str);
        }
    };

    template<class JType> struct GetConv : ValueConverter < JType > {
        //typedef ValueConverter<TesType> Conv;
    };


    template<> struct GetConv<const char*> : StringConverter{};

    //////////////////////////////////////////////////////////////////////////

    template<class T> struct j2Str {
        static function_parameter typeInfo() { return function_parameter_make(typeid(T).name(), nullptr); }
    };

    template<> struct j2Str < BSFixedString > {
        static function_parameter typeInfo() { return function_parameter_make("string", nullptr); }
    };
    template<> struct j2Str < const char* > {
        static function_parameter typeInfo() { return function_parameter_make("string", nullptr); }
    };
    template<> struct j2Str < Float32 > {
        static function_parameter typeInfo() { return function_parameter_make("float", nullptr); }
    };
    template<> struct j2Str < SInt32 > {
        static function_parameter typeInfo() { return function_parameter_make("int", nullptr); }
    };
    template<> struct j2Str < UInt32 > {
        static function_parameter typeInfo() { return function_parameter_make("int", nullptr); }
    };
    template<> struct j2Str < TESForm * > {
        static function_parameter typeInfo() { return function_parameter_make("form", nullptr); }
    };
    template<> struct j2Str < BGSListForm * > {
        static function_parameter typeInfo() { return function_parameter_make("FormList", nullptr); }
    };

    template<class T> struct j2Str < VMArray<T> > {
        static function_parameter typeInfo() {
            std::string str(j2Str<T>::typeInfo().tes_type_name);
            str += "[]";
            function_parameter info = { str, "values" };
            return info;
        }
    };

    //////////////////////////////////////////////////////////////////////////

    template <typename T, T func> struct proxy;

    template <class R, class Arg0, class Arg1, R (*func)( Arg0, Arg1 ) >
    struct proxy<R (*)(Arg0, Arg1), func>
    {
        static std::vector<type_info_func> type_strings() {
            type_info_func types[] = {
                &j2Str<R>::typeInfo,
                &j2Str<Arg0>::typeInfo,
                &j2Str<Arg1>::typeInfo,
            };
            return std::vector<type_info_func>(&types[0], &types[0] + sizeof(types)/sizeof(type_info_func));
        }

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

        static void bind(VMClassRegistry *registry, const char *className, const char *name) {
            registry->RegisterFunction(
                new NativeFunction2 <
                        StaticFunctionTag,
                        typename GetConv<R>::tes_type,
                        typename GetConv<Arg0>::tes_type,
                        typename GetConv<Arg1>::tes_type >(name, className, &tes_func, registry)
            );
        }
    };

    template <class Arg0, class Arg1, void (*func)( Arg0, Arg1 ) >
    struct proxy<void (*)(Arg0, Arg1), func>
    {
        static std::vector<type_info_func> type_strings() {
            type_info_func types[] = {
                &j2Str<void>::typeInfo,
                &j2Str<Arg0>::typeInfo,
                &j2Str<Arg1>::typeInfo,
            };
            return std::vector<type_info_func>(&types[0], &types[0] + sizeof(types)/sizeof(type_info_func));
        }

        static void tes_func(
            StaticFunctionTag* tag,
            typename GetConv<Arg0>::tes_type a0,
            typename GetConv<Arg1>::tes_type a1)
        {
            func(GetConv<Arg0>::convert2J(a0),
                    GetConv<Arg1>::convert2J(a1));
        }

        static void bind(VMClassRegistry *registry, const char *className, const char *name) {

            registry->RegisterFunction(
                new NativeFunction2 <
                StaticFunctionTag,
                void,
                typename GetConv<Arg0>::tes_type,
                typename GetConv<Arg1>::tes_type >(name, className, &tes_func, registry)
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
            static std::vector<type_info_func> type_strings() {\
                type_info_func types[] = {\
                    &j2Str<R>::typeInfo,\
                    DO_##N(TYPESTRING_NTH, NOTHING, COMA, NOTHING)\
                };\
                return std::vector<type_info_func>(&types[0], &types[0] + sizeof(types)/sizeof(type_info_func));\
            }\
            \
            static typename GetConv<R>::tes_type tes_func(     \
                StaticFunctionTag* tag     \
                DO_##N(TESPARAM_NTH, COMA, COMA, NOTHING))     \
            {     \
                return GetConv<R>::convert2Tes(func(\
                    DO_##N(TESPARAM_CONV_NTH, NOTHING, COMA, NOTHING)   \
                ));     \
            }     \
                 \
            static void bind(VMClassRegistry *registry, const char *className, const char *name) {     \
                registry->RegisterFunction(     \
                    new NativeFunction##N <StaticFunctionTag, typename GetConv<R>::tes_type   \
                        DO_##N(TESTYPE_NTH, COMA, COMA, NOTHING)  > (name, className, &tes_func, registry)   \
                );     \
            }     \
        };  \
            \
        template <DO_##N(TARGS_NTH, NOTHING, COMA, COMA) void (*func)( DO_##N(PARAM_NAMELESS_NTH, NOTHING, COMA, NOTHING) ) >     \
        struct proxy<void (*)(DO_##N(PARAM_NAMELESS_NTH, NOTHING, COMA, NOTHING)), func>     \
        {     \
            static std::vector<type_info_func> type_strings() {\
                type_info_func types[] = {\
                    &j2Str<void>::typeInfo,\
                    DO_##N(TYPESTRING_NTH, NOTHING, COMA, NOTHING)\
                };\
                return std::vector<type_info_func>(&types[0], &types[0] + sizeof(types)/sizeof(type_info_func));\
            }\
            \
            static void tes_func(     \
                StaticFunctionTag* tag     \
                DO_##N(TESPARAM_NTH, COMA, COMA, NOTHING))     \
            {     \
                func(DO_##N(TESPARAM_CONV_NTH, NOTHING, COMA, NOTHING));     \
            }     \
                 \
            static void bind(VMClassRegistry *registry, const char *className, const char *name) {     \
                registry->RegisterFunction(     \
                    new NativeFunction##N <StaticFunctionTag, void   \
                        DO_##N(TESTYPE_NTH, COMA, COMA, NOTHING)  > (name, className, tes_func, registry)   \
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

    // retrieves class_info pointer from member address
    // specially for hack made in REGISTERF macro
    inline class_info& metaInfoFromFieldAndOffset(void * fieldAddress, int offset) {
        auto mixin = (_detail::class_meta_mixin *)((char *)fieldAddress - offset);
        return mixin->metaInfo;
    }

#define REGISTER_TES_NAME(ScriptTesName)\
    struct CONCAT(_struct_, __LINE__) {\
        CONCAT(_struct_, __LINE__)() {\
            using namespace reflection;\
            auto& mInfo = binding::metaInfoFromFieldAndOffset( this, offsetof(__Type, CONCAT(_mem_, __LINE__)) );\
            mInfo._className = (ScriptTesName);\
            }\
        } CONCAT(_mem_, __LINE__);

#define REGISTERF(func, _funcname, _args, _comment)\
    struct CONCAT(_struct_, __LINE__) {\
         CONCAT(_struct_, __LINE__)() {\
             using namespace reflection;\
             function_info metaF;\
             typedef binding::proxy<decltype(binding::msvc_identity(&(func))), &(func)> binder;\
             metaF.registrator = &binder::bind;\
             metaF.param_list_func = &binder::type_strings;\
             \
             metaF.argument_names = (_args);\
             metaF.setComment(_comment);\
             metaF.name = (_funcname);\
             metaF.tes_func = &binder::tes_func;\
             auto addr = &(func);\
             metaF.c_func = addr;\
             binding::metaInfoFromFieldAndOffset(this, offsetof(__Type, CONCAT(_mem_, __LINE__))).addFunction(metaF);\
         }\
    } CONCAT(_mem_, __LINE__);

#define REGISTERF2(func, args, comment)     REGISTERF(func, #func, args, comment)

}
}
