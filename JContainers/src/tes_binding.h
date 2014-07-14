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


        struct ObjectConverter {

            typedef HandleT tes_type;

            static HandleT convert2Tes(object_base* obj) {
                return obj ? obj->tes_uid() : 0;
            }

            static object_stack_ref convert2J(HandleT hdl) {
                return tes_context::instance().getObjectRef((Handle)hdl);
            }

        };

        //////////////////////////////////////////////////////////////////////////

        template<class JType> struct GetConv : ValueConverter<JType> {
            //typedef ValueConverter<TesType> Conv;
        };

        template<> struct GetConv<const char*> : StringConverter {
        };

        template<> struct GetConv < object_stack_ref& > : ObjectConverter {};
        //template<> struct GetConv < object_stack_ref > : ObjectConverter{};

        template<> struct GetConv < object_base* > : ObjectConverter{};
        template<> struct GetConv < array* > : ObjectConverter{};
        template<> struct GetConv < map* > : ObjectConverter{};
        template<> struct GetConv < form_map* > : ObjectConverter{};


/*
        template<class T, class P> struct GetConv < boost::intrusive_ptr_jc<T, P> > : ObjectConverter{

            static boost::intrusive_ptr_jc<T, P> convert2J(HandleT hdl) {
                return tes_context::instance().getObjectRefOfType<T>((Handle)hdl);
            }

        };
*/

        template<class T, class P> struct GetConv < boost::intrusive_ptr_jc<T, P>& > : ObjectConverter{
            static boost::intrusive_ptr_jc<T, P> convert2J(HandleT hdl) {
                return tes_context::instance().getObjectRefOfType<T>((Handle)hdl);
            }
        };

        //////////////////////////////////////////////////////////////////////////

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
        template<class T, class P> struct j2Str < boost::intrusive_ptr_jc<T,P> > {
            static j_type_info typeInfo() { return j2Str<T*>::typeInfo(); }
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

            static void bind(VMClassRegistry *registry, const char *name, const char *className) {
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

            static void bind(VMClassRegistry *registry, const char *name, const char *className) {

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
            static void bind(VMClassRegistry *registry, const char *name, const char *className) {     \
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
            static void bind(VMClassRegistry *registry, const char *name, const char *className) {     \
                registry->RegisterFunction(     \
                    new NativeFunction##N <StaticFunctionTag, void   \
                        DO_##N(TESTYPE_NTH, COMA, COMA, NOTHING)  > (name, className, tes_func, registry)   \
                );     \
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
