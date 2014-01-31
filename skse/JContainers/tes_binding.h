#pragma once

#include "collections.h"

#include "skse/PapyrusVM.h"
#include "skse/PapyrusNativeFunctions.h"
#include "gtest.h"

#include <type_traits>
#include <boost/algorithm/string.hpp>
#include <sstream>

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
            std::string tes_type_name;
            std::string tes_arg_name;
        };

        inline j_type_info j_type_info_make(const char * ttn,  const char * tan) {
            j_type_info info = {ttn ? ttn : "", tan ? tan : ""};
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
                 printf("register func %s for %s\n", name, className);

                if (!registry) return;

                registry->RegisterFunction(
                    new NativeFunction2 <
                            StaticFunctionTag,
                            typename J2Tes<R>::tes_type,
                            typename J2Tes<Arg0>::tes_type,
                            typename J2Tes<Arg1>::tes_type > (name, className, &tes_func, registry)
                );

                registry->SetFunctionFlags(className, name, VMClassRegistry::kFunctionFlag_NoWait); \
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

                //printf("%s %s\n", name, typeid(decltype(tes_func)).name());
                printf("register func %s for %s\n", name, className);

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
                printf("register func %s for %s\n", name, className);\
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
                printf("register func %s for %s\n", name, className);\
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
            typedef  std::vector<type_info_func> (*TypeStrings)();

            Registrator registrator;
            TypeStrings typeStrings;
            const char *comment;
            const char *args;
            const char *funcName;

            FunctionMetaInfo() {
                memset(this, 0, sizeof(*this));
            }

            void bind(VMClassRegistry *registry, const char *className) {
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

                    auto argName = types[paramIdx + 1]().tes_arg_name; 
                    if (argName.empty() == false) {
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

                str += "\n;/";
                str += comment;
                str += "\n/;\n";
            }

            std::string function_string() const {
                std::string str;
                auto types = typeStrings();

                _pushComment(str);

                if (types[0]().tes_type_name != "void") {
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


        struct class_meta_info {
            std::vector<FunctionMetaInfo> methods;
            const char *className;
            const char *extendsClass;
            bool initialized;

            class_meta_info() : className("NoClassName"), extendsClass(NULL), initialized(false) {}
        };

        template<class T>
        struct class_meta_mixin {

            static class_meta_info& metaInfo() {
                static class_meta_info info;
                if (!info.initialized) {
                    info.initialized = true;

                    T t;

                    T::additionalSetup();
                }

                return info;
            }

            static void additionalSetup() {}

            static void register_me(FunctionMetaInfo *info) {
                metaInfo().methods.push_back(*info);
                //printf("func %s registered\n", info->function_string().c_str());
            }

            static std::string produceTesCode() {
                std::stringstream stream;
                stream << "Scriptname " << metaInfo().className;
                if (metaInfo().extendsClass) {
                    stream << " extends " << metaInfo().extendsClass;
                }
                stream << " Hidden" << std::endl << std::endl;

                for (auto itm : metaInfo().methods) {
                    stream << itm.function_string() << std::endl;
                }

                return stream.str();
            }

            static void writeToFile() {
                auto file = fopen((std::string(metaInfo().className) + ".psc").c_str(), "w");
                if (file) {
                    auto code = produceTesCode();
                    fwrite(code.c_str(), 1, code.length(), file);
                    fclose(file);
                }
            }

            static void bind(VMClassRegistry* registry) {
                assert(metaInfo().className);

                writeToFile();

                printf("%s\n", produceTesCode().c_str());

                for (auto itm : metaInfo().methods) {
                    itm.bind(registry, metaInfo().className);
                }
            }
        };
    }

#define CONCAT(x, y) CONCAT1 (x, y)
#define CONCAT1(x, y) x##y

#define STR(...)    #__VA_ARGS__

    // MSVC2012 bug workaround
    template <typename T> T msvc_identity(T);

#define REGISTER_TES_NAME(ScriptTesName)\
    struct CONCAT(_struct_, __LINE__) {\
        CONCAT(_struct_, __LINE__)() {\
            metaInfo().className = (ScriptTesName);\
        }\
    } CONCAT(_mem_, __LINE__);

#define REGISTERF(func, _funcname, _args, _comment)\
    struct CONCAT(_struct_, __LINE__) : public tes_binding::FunctionMetaInfo {\
         CONCAT(_struct_, __LINE__)() {\
             registrator = &tes_binding::proxy<decltype(msvc_identity(&(func))), &(func)>::bind;\
             typeStrings = &tes_binding::proxy<decltype(msvc_identity(&(func))), &(func)>::type_strings;\
             \
             args = _args;\
             comment = _comment;\
             funcName = _funcname;\
             register_me(this);\
         }\
    } CONCAT(_mem_, __LINE__);

#define REGISTERF2(func, args, comment)     REGISTERF(func, #func, args, comment)

}
