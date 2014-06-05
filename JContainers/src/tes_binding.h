#pragma once

#include "skse/PapyrusNativeFunctions.h"

#include "tes_meta_info.h"
#include "collections.h"
#include "tes_context.h"
#include "code_producer.h"

namespace collections {

    class map;
    class form_map;
    class array;
    class object_base;

#define NOTHING 

#define DO_0(rep, f, m, l)
#define DO_1(rep, f, m_, l)   f rep(1) l
#define DO_2(rep, f, m, l)   DO_1(rep, f, m, NOTHING) m  rep(2) l
#define DO_3(rep, f, m, l)   DO_2(rep, f, m, NOTHING) m  rep(3) l
#define DO_4(rep, f, m, l)   DO_3(rep, f, m, NOTHING) m  rep(4) l
#define DO_5(rep, f, m, l)   DO_4(rep, f, m, NOTHING) m  rep(5) l

    namespace tes_binding {

        template<class TesType>
        struct Tes2J {
            typedef TesType j_type;
        };

        template<class J>
        struct J2Tes {
            typedef J tes_type;
        };

        template<class Out, class In> inline Out convert2J(In in) {
            return in;
        }

        template<class Out, class In> inline Out convert2Tes(In in) {
            return in;
        }

        template<>
        struct Tes2J<BSFixedString> {
            typedef const char* j_type;
        };
        template<>
        struct J2Tes<const char*> {
            typedef BSFixedString tes_type;
        };
        template<> inline BSFixedString convert2Tes<BSFixedString>(const char * str) {
            return BSFixedString(str);
        }
        template<> inline const char* convert2J(const BSFixedString& str) {
            return str.data;
        }
        template<> inline const char* convert2J(BSFixedString str) {
            return str.data;
        }

        template<>
        struct J2Tes<object_base*> {
            typedef HandleT tes_type;
        };
        template<> inline HandleT convert2Tes(object_base* obj) {
            return obj ? obj->uid() : 0;
        }
        template<> inline HandleT convert2Tes(array* obj) {
            return obj ? obj->uid() : 0;
        }
        template<> inline HandleT convert2Tes(map* obj) {
            return obj ? obj->uid() : 0;
        }
        template<> inline HandleT convert2Tes(form_map* obj) {
            return obj ? obj->uid() : 0;
        }

        template<>
        struct Tes2J<HandleT> {
            typedef object_base* j_type;
        };

        template<> struct J2Tes<array*> {
            typedef HandleT tes_type;
        };
        template<> struct J2Tes<map*> {
            typedef HandleT tes_type;
        };
        template<> struct J2Tes<form_map*> {
            typedef HandleT tes_type;
        };
        template<> inline object_base* convert2J(HandleT hdl) {
            return tes_context::instance().getObject(hdl);
        }
        template<> inline array* convert2J(HandleT hdl) {
            return tes_context::instance().getObjectOfType<array>(hdl);
        }
        template<> inline map* convert2J(HandleT hdl) {
            return tes_context::instance().getObjectOfType<map>(hdl);
        }
        template<> inline form_map* convert2J(HandleT hdl) {
            return tes_context::instance().getObjectOfType<form_map>(hdl);
        }
        template<>
        struct J2Tes<Handle> {
            typedef HandleT tes_type;
        };
        template<> inline HandleT convert2Tes(Handle hdl) {
            return (HandleT)hdl;
        }

        template<class T> struct j2Str {
            static j_type_info typeInfo() { return j_type_info_make(typeid(T).name(), nullptr); }
        };

        template<> struct j2Str<object_base *> {
            static j_type_info typeInfo() { return j_type_info_make("int", "object"); }
        };
        template<> struct j2Str<map *> {
            static j_type_info typeInfo() { return j_type_info_make("int", "object"); }
        };
        template<> struct j2Str<array *> {
            static j_type_info typeInfo() { return j_type_info_make("int", "object"); }
        };
        template<> struct j2Str<form_map *> {
            static j_type_info typeInfo() { return j_type_info_make("int", "object"); }
        };
        template<> struct j2Str<HandleT> {
            static j_type_info typeInfo() { return j_type_info_make("int", "object"); }
        };
        template<> struct j2Str<Handle> {
            static j_type_info typeInfo() { return j_type_info_make("int", "object"); }
        };

        template<> struct j2Str<BSFixedString> {
            static j_type_info typeInfo() { return j_type_info_make("string", nullptr); }
        };
        template<> struct j2Str<const char*> {
            static j_type_info typeInfo() { return j_type_info_make("string", nullptr); }
        };
        template<> struct j2Str<Float32> {
            static j_type_info typeInfo() { return j_type_info_make("float", nullptr); }
        };
        template<> struct j2Str<SInt32> {
            static j_type_info typeInfo() { return j_type_info_make("int", nullptr); }
        };
        template<> struct j2Str<TESForm *> {
            static j_type_info typeInfo() { return j_type_info_make("form", nullptr); }
        };

        template<class T> struct j2Str<VMArray<T> > {
            static j_type_info typeInfo() {
                std::string str( j2Str<T>::typeInfo().tes_type_name );
                str += "[]";
                j_type_info info = {str, "values"};
                return info;
            }
        };


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

            static typename J2Tes<R>::tes_type tes_func(
                StaticFunctionTag* tag,
                typename J2Tes<Arg0>::tes_type a0,
                typename J2Tes<Arg1>::tes_type a1)
            {
                return convert2Tes<J2Tes<R>::tes_type, R>(
                    func(
                        convert2J<Arg0>(a0),
                        convert2J<Arg1>(a1))
                );
            }

            static void bind(VMClassRegistry *registry, const char *name, const char *className) {
                 //printf("%s %s\n", name, typeid(decltype(tes_func)).name());

                if (!registry) return;

                registry->RegisterFunction(
                    new NativeFunction2 <
                            StaticFunctionTag,
                            typename J2Tes<R>::tes_type,
                            typename J2Tes<Arg0>::tes_type,
                            typename J2Tes<Arg1>::tes_type > (name, className, &tes_func, registry)
                );

                registry->SetFunctionFlags(className, name, VMClassRegistry::kFunctionFlag_NoWait);
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
                typename J2Tes<Arg0>::tes_type a0,
                typename J2Tes<Arg1>::tes_type a1)
            {
                func(convert2J<Arg0>(a0), convert2J<Arg1>(a1));
            }

            static void bind(VMClassRegistry *registry, const char *name, const char *className) {


                if (!registry) return;

                registry->RegisterFunction(
                    new NativeFunction2 <
                    StaticFunctionTag,
                    void,
                    typename J2Tes<Arg0>::tes_type,
                    typename J2Tes<Arg1>::tes_type > (name, className, &tes_func, registry)
                    );

                registry->SetFunctionFlags(className, name, VMClassRegistry::kFunctionFlag_NoWait); \
            }
        };

    #define TARGS_NTH(n)            class Arg ## n
    #define PARAM_NTH(n)            Arg##n arg##n
    #define PARAM_NAMELESS_NTH(n)    Arg##n
    #define ARG_NTH(n)              a##n
    #define COMA                    ,

    #define TESTYPE_NTH(n)     typename J2Tes<Arg##n>::tes_type
    #define TESPARAM_NTH(n)     TESTYPE_NTH(n) a##n
    #define TESPARAM_CONV_NTH(n)     convert2J<Arg##n>(a##n)
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
            static typename J2Tes<R>::tes_type tes_func(     \
                StaticFunctionTag* tag     \
                DO_##N(TESPARAM_NTH, COMA, COMA, NOTHING))     \
            {     \
                return convert2Tes<J2Tes<R>::tes_type, R> ( func(    \
                    DO_##N(TESPARAM_CONV_NTH, NOTHING, COMA, NOTHING)   \
                ));     \
            }     \
                 \
            static void bind(VMClassRegistry *registry, const char *name, const char *className) {     \
                if (!registry) return;\
                registry->RegisterFunction(     \
                    new NativeFunction##N <StaticFunctionTag, typename J2Tes<R>::tes_type   \
                        DO_##N(TESTYPE_NTH, COMA, COMA, NOTHING)  > (name, className, &tes_func, registry)   \
                );     \
                registry->SetFunctionFlags(className, name, VMClassRegistry::kFunctionFlag_NoWait); \
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
            static void bind(VMClassRegistry *registry, const char *name, const char *className) {     \
                if (!registry) return;\
                registry->RegisterFunction(     \
                    new NativeFunction##N <StaticFunctionTag, void   \
                        DO_##N(TESTYPE_NTH, COMA, COMA, NOTHING)  > (name, className, tes_func, registry)   \
                );     \
                registry->SetFunctionFlags(className, name, VMClassRegistry::kFunctionFlag_NoWait); \
            }     \
        };


        MAKE_PROXY_NTH(0);
        MAKE_PROXY_NTH(1);
        //MAKE_PROXY_NTH(2);
        MAKE_PROXY_NTH(3);
        MAKE_PROXY_NTH(4);
   }

#define CONCAT(x, y) CONCAT1 (x, y)
#define CONCAT1(x, y) x##y

#define STR(...)    #__VA_ARGS__

    // MSVC2012 bug workaround
    template <typename T> T msvc_identity(T);

    // specially for hack made in REGISTERF macro
    namespace tes_binding {
        inline class_meta_info& metaInfoFromFieldAndOffset(void * fieldAddress, int offset) {
            auto mixin = (class_meta_mixin *)((char *)fieldAddress - offset);
            return mixin->metaInfo;
        }
    }

#define REGISTER_TES_NAME(ScriptTesName)\
    struct CONCAT(_struct_, __LINE__) {\
        CONCAT(_struct_, __LINE__)() {\
            auto& mInfo = tes_binding::metaInfoFromFieldAndOffset( this, offsetof(__Type, CONCAT(_mem_, __LINE__)) );\
            mInfo.className = (ScriptTesName);\
        }\
    } CONCAT(_mem_, __LINE__);

#define REGISTERF(func, _funcname, _args, _comment)\
    struct CONCAT(_struct_, __LINE__) {\
         CONCAT(_struct_, __LINE__)() {\
             tes_binding::FunctionMetaInfo metaF;\
             typedef tes_binding::proxy<decltype(msvc_identity(&(func))), &(func)> binder;\
             metaF.registrator = &binder::bind;\
             metaF.typeStrings = &binder::type_strings;\
             \
             metaF.args = (_args);\
             metaF.setComment(_comment);\
             metaF.funcName = (_funcname);\
             tes_binding::metaInfoFromFieldAndOffset(this, offsetof(__Type, CONCAT(_mem_, __LINE__))).addFunction(metaF);\
         }\
    } CONCAT(_mem_, __LINE__);

#define REGISTERF2(func, args, comment)     REGISTERF(func, #func, args, comment)

}
