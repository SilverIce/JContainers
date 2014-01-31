#pragma once

#include "collections.h"

#include "skse/PapyrusVM.h"
#include "skse/PapyrusNativeFunctions.h"
#include "gtest.h"

#include <type_traits>
#include <boost/algorithm/string.hpp>

namespace collections {

    class map;
    class form_map;
    class array;
    class object_base;

#define NOTHING 

#define DO_0(rep, f, m, l)
#define DO_1(rep, f, m, l)   f rep(1) l
#define DO_2(rep, f, m, l)   DO_1(rep, f, m, NOTHING) m  rep(2) l
#define DO_3(rep, f, m, l)   DO_2(rep, f, m, NOTHING) m  rep(3) l
#define DO_4(rep, f, m, l)   DO_3(rep, f, m, NOTHING) m  rep(4) l
#define DO_5(rep, f, m, l)   DO_4(rep, f, m, NOTHING) m  rep(5) l

    namespace tes_traits {

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
        template<> BSFixedString convert2Tes<BSFixedString>(const char * str) {
            return BSFixedString(str);
        }
        template<> const char* convert2J(const BSFixedString& str) {
            return str.data;
        }
        template<> const char* convert2J(BSFixedString str) {
            return str.data;
        }

        template<>
        struct J2Tes<object_base*> {
            typedef HandleT tes_type;
        };
        template<> HandleT convert2Tes(object_base* obj) {
            return obj ? obj->id : 0;
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
        template<> object_base* convert2J(HandleT hdl) {
            return collection_registry::getObject(hdl);
        }
        template<> array* convert2J(HandleT hdl) {
            return collection_registry::getObjectOfType<array>(hdl);
        }
        template<> map* convert2J(HandleT hdl) {
            return collection_registry::getObjectOfType<map>(hdl);
        }
        template<> form_map* convert2J(HandleT hdl) {
            return collection_registry::getObjectOfType<form_map>(hdl);
        }
        template<>
        struct J2Tes<Handle> {
            typedef HandleT tes_type;
        };
        template<> HandleT convert2Tes(Handle hdl) {
            return (HandleT)hdl;
        }


        struct j_type_info {
            const char * tes_type_name;
            const char * tes_arg_name;
        };

        inline j_type_info j_type_info_make(const char * ttn,  const char * tan) {
            j_type_info info = {ttn, tan};
            return info;
        }

        typedef j_type_info (*type_info_func)();

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
    }

    template <typename T, T> struct proxy;


    template <class R, class Arg0, class Arg1, R (*func)( Arg0, Arg1 ) >
    struct proxy<R (*)(Arg0, Arg1), func>
    {
        static std::vector<tes_traits::type_info_func> type_strings() {
            tes_traits::type_info_func types[] = {
                &tes_traits::j2Str<R>::typeInfo,
                &tes_traits::j2Str<Arg0>::typeInfo,
                &tes_traits::j2Str<Arg1>::typeInfo,
            };
            return std::vector<tes_traits::type_info_func>(&types[0], &types[0] + sizeof(types)/sizeof(tes_traits::type_info_func));
        }

        static typename tes_traits::J2Tes<R>::tes_type tes_func(
            StaticFunctionTag* tag,
            typename tes_traits::J2Tes<Arg0>::tes_type a0,
            typename tes_traits::J2Tes<Arg1>::tes_type a1)
        {
            return tes_traits::convert2Tes<R>(
                func(
                    tes_traits::convert2J<Arg0>(a0),
                    tes_traits::convert2J<Arg1>(a1))
            );
        }

        static void bind(VMClassRegistry *registry, const char *name, const char *className) {
            registry->RegisterFunction(
                new NativeFunction2 <
                        StaticFunctionTag,
                        typename tes_traits::J2Tes<R>::tes_type,
                        typename tes_traits::J2Tes<Arg0>::tes_type,
                        typename tes_traits::J2Tes<Arg1>::tes_type > (name, className, &tes_func, registry)
            );
        }
    };

    template <class Arg0, class Arg1, void (*func)( Arg0, Arg1 ) >
    struct proxy<void (*)(Arg0, Arg1), func>
    {
        static std::vector<tes_traits::type_info_func> type_strings() {
            tes_traits::type_info_func types[] = {
                &tes_traits::j2Str<void>::typeInfo,
                &tes_traits::j2Str<Arg0>::typeInfo,
                &tes_traits::j2Str<Arg1>::typeInfo,
            };
            return std::vector<tes_traits::type_info_func>(&types[0], &types[0] + sizeof(types)/sizeof(tes_traits::type_info_func));
        }

        static void tes_func(
            StaticFunctionTag* tag,
            typename tes_traits::J2Tes<Arg0>::tes_type a0,
            typename tes_traits::J2Tes<Arg1>::tes_type a1)
        {
            func(tes_traits::convert2J<Arg0>(a0), tes_traits::convert2J<Arg1>(a1));
        }

        static void bind(VMClassRegistry *registry, const char *name, const char *className) {
            registry->RegisterFunction(
                new NativeFunction2 <
                StaticFunctionTag,
                void,
                typename tes_traits::J2Tes<Arg0>::tes_type,
                typename tes_traits::J2Tes<Arg1>::tes_type > (name, className, &tes_func, registry)
                );
        }
    };

#define TARGS_NTH(n)            class Arg ## n
#define PARAM_NTH(n)            Arg##n arg##n
#define PARAM_NAMELESS_NTH(n)    Arg##n
#define ARG_NTH(n)              a##n
#define COMA                    ,

#define TESTYPE_NTH(n)     typename tes_traits::J2Tes<Arg##n>::tes_type
#define TESPARAM_NTH(n)     TESTYPE_NTH(n) a##n
#define TESPARAM_CONV_NTH(n)     tes_traits::convert2J<Arg##n>(a##n)
#define TYPESTRING_NTH(n)     &tes_traits::j2Str<Arg##n>::typeInfo





#define MAKE_PROXY_NTH(N) \
    template <class R DO_##N(TARGS_NTH, COMA, COMA, NOTHING), R (*func)( DO_##N(PARAM_NAMELESS_NTH, NOTHING, COMA, NOTHING) ) >     \
    struct proxy<R (*)(DO_##N(PARAM_NAMELESS_NTH, NOTHING, COMA, NOTHING)), func>     \
    {     \
        static std::vector<tes_traits::type_info_func> type_strings() {\
            tes_traits::type_info_func types[] = {\
                &tes_traits::j2Str<R>::typeInfo,\
                DO_##N(TYPESTRING_NTH, NOTHING, COMA, NOTHING)\
            };\
            return std::vector<tes_traits::type_info_func>(&types[0], &types[0] + sizeof(types)/sizeof(tes_traits::type_info_func));\
        }\
        \
        static typename tes_traits::J2Tes<R>::tes_type tes_func(     \
            StaticFunctionTag* tag     \
            DO_##N(TESPARAM_NTH, COMA, COMA, NOTHING))     \
        {     \
            return tes_traits::convert2Tes<R>( func(    \
                DO_##N(TESPARAM_CONV_NTH, NOTHING, COMA, NOTHING)   \
            ));     \
        }     \
             \
        static void bind(VMClassRegistry *registry, const char *name, const char *className) {     \
            registry->RegisterFunction(     \
                new NativeFunction##N <StaticFunctionTag, typename tes_traits::J2Tes<R>::tes_type   \
                    DO_##N(TESTYPE_NTH, COMA, COMA, NOTHING)  > (name, className, &tes_func, registry)   \
            );     \
        }     \
    };  \
        \
    template <DO_##N(TARGS_NTH, NOTHING, COMA, COMA) void (*func)( DO_##N(PARAM_NAMELESS_NTH, NOTHING, COMA, NOTHING) ) >     \
    struct proxy<void (*)(DO_##N(PARAM_NAMELESS_NTH, NOTHING, COMA, NOTHING)), func>     \
    {     \
        static std::vector<tes_traits::type_info_func> type_strings() {\
            tes_traits::type_info_func types[] = {\
                &tes_traits::j2Str<void>::typeInfo,\
                DO_##N(TYPESTRING_NTH, NOTHING, COMA, NOTHING)\
            };\
            return std::vector<tes_traits::type_info_func>(&types[0], &types[0] + sizeof(types)/sizeof(tes_traits::type_info_func));\
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

/*
#define MAKE_PROXY_NTH(N) \
    template < DO_##N (TARGS_NTH, NOTHING, COMA, NOTHING) >     \
    struct proxyfake3 \
    {     \
    };*/


    MAKE_PROXY_NTH(0);
    MAKE_PROXY_NTH(1);
    //MAKE_PROXY_NTH(2);
    MAKE_PROXY_NTH(3);
    MAKE_PROXY_NTH(4);
    

    void tes_fake_func(array *, Float32) {
      //  return BSFixedString(NULL);
    }

    TEST(proxy, proxy) {
        &proxy<decltype(&tes_fake_func), &tes_fake_func>:: tes_func;
        &proxy<decltype(&tes_fake_func), &tes_fake_func>:: bind;
        proxy<decltype(&tes_fake_func), &tes_fake_func>:: type_strings();
    }

    struct FunctionMetaInfo {
        typedef  void (*Registrator)(VMClassRegistry* registry, const char*, const char*);
        typedef  std::vector<tes_traits::type_info_func> (*TypeStrings)();

        Registrator registrator;
        TypeStrings typeStrings;
        const char *comment;
        const char *args;
        const char *funcName;

        FunctionMetaInfo() {
            memset(this, 0, sizeof(*this));
        }

        void bind(VMClassRegistry *registry, const char *className) {
            if (!registry) return;
            registrator(registry, funcName, className);
        }

        void _pushArgStr(int paramIdx, std::string& str) const {
            vector<string> strings; // #2: Search for tokens
            boost::split( strings, string(args), boost::is_space() );

            if (paramIdx < strings.size() && strings[paramIdx] != "*") {
                str += strings[paramIdx];
            }
            else {
                auto types = typeStrings();

                auto argName = types[paramIdx]().tes_arg_name; 
                if (argName) {
                    str += argName;
                } else {
                    str += "arg";
                    str += (char)(paramIdx + '0');
                }
            }
        }

        void _pushComment(std::string& str) const {
            if (!comment || !*comment) {
                return ;
            }

            str += ";/";
            str += comment;
            str += "\n/;\n";
        }

        std::string function_string() const {
            std::string str;
            auto types = typeStrings();

            _pushComment(str);

            if (strcmp(types[0]().tes_type_name, "void") != 0) {
                str += types[0]().tes_type_name;
                str += ' ';
            }

            str += "Function ";
            str += funcName;
            str += '(';
            for (int i = 1; i < types.size(); ++i) {
                str += types[i]().tes_type_name;
                str += " ";

                int paramIdx = i - 1;
                _pushArgStr(paramIdx, str);

                if (i < (types.size() - 1))
                    str += ", ";
            }

            str += ") global native";
            return str;
        }
    };

#define CONCAT(x, y) CONCAT1 (x, y)
#define CONCAT1(x, y) x##y

#define STR(...)    #__VA_ARGS__

    // MSVC2012 bug workaround
    template <typename T> T msvc_identity(T);
    //decltype(identity(&convert<int>));

#define REGISTERF(func, _funcname, _args, _comment)\
    struct CONCAT(_struct_, __LINE__) : public FunctionMetaInfo {\
         CONCAT(_struct_, __LINE__)() {\
             registrator = &proxy<decltype(msvc_identity(&(func))), &(func)>::bind;\
             typeStrings = &proxy<decltype(msvc_identity(&(func))), &(func)>::type_strings;\
             \
             args = _args;\
             comment = _comment;\
             funcName = _funcname;\
             register_me(this);\
         }\
    } CONCAT(_mem_, __LINE__);

#define REGISTERF2(func, args, comment)     REGISTERF(func, #func, args, comment)

}
