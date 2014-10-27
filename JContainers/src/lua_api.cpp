#pragma once

//#include "lua_stuff.h"

#include <utility>
#include <string>
#include <boost/optional.hpp>
#include "boost/filesystem/path.hpp"
#include <sstream>

#include "gtest.h"
#include "util.h"

#include "reflection.h"
#include "lua_basics.h"

#include "jcontainers_constants.h"
#include "collections.h"
#include "tes_context.h"
#include "collection_functions.h"

namespace {

    using namespace collections;

#   define cexport extern "C" __declspec(dllexport)

    namespace { // a trick to shut-up compiler
#   include "Data/SKSE/Plugins/JCData/InternalLuaScripts/api_for_lua.h"
    }

    cexport const char* JC_hello() {
        return "lua, are you there?";
    }

    cexport void* JC_get_c_function(const char *functionName, const char * className) {
        using namespace reflection;

        c_function functionPtr = nullptr;
        auto& db = class_database();
        auto itr = db.find(className);
        if (itr != db.end()) {
            auto& cls = itr->second;

            if (const function_info* fInfo = cls.find_function(functionName)) {
                functionPtr = fInfo->c_func;
            }
        }

        return functionPtr;
    }

    JCToLuaValue JCToLuaValue_None() {
        return{ item_type::no_item, { 0 }, 0 };
    }

    cexport void JCToLuaValue_free(JCToLuaValue* v) {
        if (v && v->type == item_type::string) {
            free((void *)v->string);
        }
    }

    cexport void CString_free(CString *str) {
        if (str) {
            free((void *)str->str);
        }
    }

    CString CString_None() {
        return{ nullptr, 0 };
    }

    CString CString_copy(const char *origin, size_t length = -1) {
        if (!origin) {
            return CString_None();
        }

        length = length != -1 ? length : strlen(origin);
        char *string = (char *)calloc(length + 1, sizeof(char));
        strcpy(string, origin);
        return{ string, length };
    }


    CString CString_copy(const std::string& origin) {
        return CString_copy(origin.c_str(), origin.size());
    }

    JCToLuaValue JCToLuaValue_fromItem(const Item& itm) {
        struct t : public boost::static_visitor < > {
            JCToLuaValue value;

            void operator ()(const std::string& str) {
                value.string = CString_copy(str.c_str(), str.size()).str;
                value.stringLength = str.size();
            }

            void operator ()(const Item::Real& val) {
                value.real = val;
            }

            void operator ()(const SInt32& val) {
                value.integer = val;
            }

            void operator ()(const boost::blank&) {}

            void operator ()(const collections::internal_object_ref& val) {
                value.object = { val.get() };
            }

            void operator ()(const FormId& val) {
                value.form = { val };
            }

        } converter;

        converter.value.type = itm.type();
        itm.var().apply_visitor(converter);
        return converter.value;
    }

    void JCValue_fillItem(const JCValue *v, Item& itm) {
        switch (v ? v->type : item_type::no_item) {
        case item_type::form:
            itm = (FormId)v->form.___id;
            break;
        case item_type::integer:
            itm = v->integer;
            break;
        case item_type::real:
            itm = v->real;
            break;
        case item_type::object:
            itm = (object_base *)v->object;
            break;
        case item_type::string:
            itm = v->string;
            break;
        case item_type::no_item:
        case item_type::none:
            itm = boost::blank();
            break;
        default:
            assert(false);
            break;
        }
    }

    template<class JCV> std::string JCValue_toString(const JCV *v) {
        std::stringstream str;
        if (!v) {
            str << "nil";
        }
        else {
            switch (v->type)
            {
            case collections::no_item:
                str << "no_item";
                break;
            case collections::none:
                str << "none";
                break;
            case collections::integer:
                str << "integer " << v->integer;
                break;
            case collections::real:
                str << "real " << v->real;
                break;
            case collections::form:
                str << "form " << v->form;
                break;
            case collections::object:
                str << "object " << v->object;
                break;
            case collections::string:
                str << "string " << (const char*)v->string;
                break;
            default:
                assert(false);
                break;
            }
        }

        return str.str();
    }
    template<class JCV> std::string JCValue_toString(const JCV &v) { return JCValue_toString(&v); }

    cexport handle JValue_retain(object_base* obj) { return (obj ? obj->stack_retain(), obj : nullptr); }
    cexport handle JValue_release(object_base* obj) { return (obj ? obj->stack_release(), nullptr : nullptr); }
    cexport collections::CollectionType JValue_typeId(object_base* obj) { return (obj ? obj->_type : CollectionTypeNone); }

    cexport JCToLuaValue JArray_getValue(array* obj, index key) {
        JCToLuaValue v(JCToLuaValue_None());
        array_functions::doReadOp(obj, key, [=, &v](index idx) {
            v = JCToLuaValue_fromItem(obj->u_container()[idx]);
        });
        //std::cout << "value returned: " << JCValue_toString(v) << std::endl;
        return v;
    }

    cexport void JArray_setValue(array* obj, index key, const JCValue* val) {
        array_functions::doReadOp(obj, key, [=](index idx) {
            JCValue_fillItem(val, obj->u_container()[idx]);
        });
        //std::cout << "value assigned: " << JCValue_toString(val) << std::endl;
    }

    cexport void JArray_insert(array* obj, const JCValue* val, index key) {
        array_functions::doWriteOp(obj, key, [=](index idx) {
            auto& cnt = obj->u_container();
            JCValue_fillItem(val, *cnt.insert(cnt.begin() + idx, Item()));
        });
        //std::cout << "value assigned: " << JCValue_toString(val) << std::endl;
    }

    //////////////////////////////////////////////////////////////////////////
    cexport CString JMap_nextKey(const map *obj, cstring lastKey) {
        CString next = CString_None();

        if (obj) {
            object_lock g(obj);
            auto& container = obj->u_container();
            if (lastKey) {
                auto itr = container.find(lastKey);
                auto end = container.end();
                if (itr != end && (++itr) != end) {
                    next = CString_copy(itr->first);
                }
            }
            else if (container.empty() == false) {
                next = CString_copy(container.begin()->first);
            }
        }
        return next;
    }

    cexport void JMap_setValue(map *obj, cstring key, const JCValue* val) {
        map_functions::doWriteOp(obj, key, [val](Item& itm) { JCValue_fillItem(val, itm); });
    }

    cexport JCToLuaValue JMap_getValue(map *obj, cstring key) {
        return map_functions::doReadOpR(obj, key, JCToLuaValue_None(), [](Item& itm) { return JCToLuaValue_fromItem(itm); });
    }
    //////////////////////////////////////////////////////////////////////////

    static_assert(sizeof FormId == sizeof CForm, "");

    cexport FormId JFormMap_nextKey(const form_map *obj, FormId lastKey) {
        FormId next = FormZero;

        if (obj) {
            object_lock g(obj);
            auto& container = obj->u_container();
            if (lastKey) {
                auto itr = container.find(lastKey);
                auto end = container.end();
                if (itr != end && (++itr) != end) {
                    next = itr->first;
                }
            }
            else if (container.empty() == false) {
                next = container.begin()->first;
            }
        }
        return next;
    }

    cexport void JFormMap_setValue(form_map *obj, FormId key, const JCValue* val) {
        formmap_functions::doWriteOp(obj, key, [val](Item& itm) { JCValue_fillItem(val, itm); });
    }

    cexport JCToLuaValue JFormMap_getValue(form_map *obj, FormId key) {
        return formmap_functions::doReadOpR(obj, key, JCToLuaValue_None(), [](Item& itm) { return JCToLuaValue_fromItem(itm); });
    }

    cexport handle JDB_instance() {
        return tes_context::instance().database();
    }

}

namespace collections {

    namespace {

        void printLuaError(lua_State *l) {
            if (lua_gettop(l) > 0 && lua_isstring(l, -1)) {
                const char *str = lua_tostring(l, -1);
                _MESSAGE("Lua error: %s", str ? str : "");
                skse::console_print(str);
            }
        }

        int LuaErrorHandler(lua_State *l) {
            
            int message = lua_gettop(l);//1
            printLuaError(l);

            lua_getglobal(l, "debug");//2
            lua_pushstring(l, "traceback");
            lua_gettable(l, 2); 
            int function = lua_gettop(l);

            lua_call(l, 0, 1);

            printLuaError(l);

            lua_pushvalue(l, message);
            return 1;
        }

        bool setupLuaContext(lua_State *l) {
            typedef boost::filesystem::path path;

            auto initScriptPath = util::relative_to_dll_path(JC_DATA_FILES "InternalLuaScripts/init.lua");

            lua_pushcfunction(l, LuaErrorHandler);
            int errorHandler = lua_gettop(l);

            if (luaL_loadfile(l, initScriptPath.generic_string().c_str()) != lua::LUA_OK) {
                printLuaError(l);
                return false;
            }

            // push data path, dll path
            lua_pushstring(l, util::relative_to_dll_path(JC_DATA_FILES).generic_string().c_str());
            lua_pushstring(l, util::dll_path().generic_string().c_str());

            if (lua_pcall(l, 2, LUA_MULTRET, errorHandler) != lua::LUA_OK) {
                printLuaError(l);
                return false;
            }

            return true;
        }

        void _setupLuaContext(lua_State *l) { setupLuaContext(l); }

        LUA_CONTEXT_MODIFIER(_setupLuaContext);

        boost::optional<Item> eval_lua_function_for_test(lua_State *L, object_base *object, const char *lua_string) {

            lua_pushcfunction(L, LuaErrorHandler);
            int errorHandler = lua_gettop(L);

            lua_getglobal(L, "JC_compileAndRun");
            lua_pushstring(L, lua_string);
            lua_pushlightuserdata(L, object);

            if (lua_pcall(L, 2, 1, errorHandler) != lua::LUA_OK) {
                return boost::none;
            }
            else {
                Item result;
                JCValue *val = (JCValue *)lua_topointer(L, -1);
                JCValue_fillItem(val, result);
                return result;
            }
        }

        TEST_F(lua::fixture, Lua, evalLua_imitation)
        {
            EXPECT_TRUE(setupLuaContext(l));

/*
            auto result = eval_lua_function_for_test(l, &array::object(tes_context::instance()),
                "assert(jobject);"
                "local obj = JArray.objectWithArray {1,2,3,4,9};"
                "return jc.filter(obj, function(x) return x < 3.5 end);"
                );

            EXPECT_TRUE(result);

            auto obj = result->object();
            EXPECT_NOT_NIL(obj);
            auto filtered = result->object()->as<array>();
            EXPECT_NOT_NIL(filtered);
            EXPECT_TRUE(filtered->s_count() == 3);*/

            auto testTransporting = [&](const char *str) { return eval_lua_function_for_test(l, nullptr, str); };

            EXPECT_TRUE(testTransporting("return 10")->intValue() == 10);
            EXPECT_TRUE(testTransporting("return 'die'")->strValue() == std::string("die"));
            EXPECT_TRUE(testTransporting("return Form(20)")->formId() == FormId(20));


            auto db = tes_context::instance().database();

            EXPECT_TRUE(testTransporting("return JDB")->object() == db);

            db->setValueForKey("test", Item(10));
            EXPECT_TRUE(db->find("test").intValue() == 10);

            auto result = eval_lua_function_for_test(l, nullptr,
                "for k,v in pairs(JDB) do print('JDB.test', k, v) end;"
                "assert(JDB.test == 10);"
                "return JDB.test"
                );
            EXPECT_TRUE(result.is_initialized());
            EXPECT_TRUE(result->intValue() == 10);

        }
    }

    boost::optional<Item> eval_lua_function(object_base *object, const char *lua_string) {
        return eval_lua_function_for_test(lua::context::instance().state(), object, lua_string);
    }
}

#if 0
namespace collections { namespace {

    template<class T> struct trait;

    template<class T> struct trait_base {

        typedef trait<T> self_type;

        static const char *table_name() {
            return typeid(self_type).name();
        }
    };

    template<>
    struct trait < std::string > {
        static void push(lua_State *l, const std::string& str) {
            lua_pushlstring(l, str.c_str(), str.size());
        }
    };

    template<>
    struct trait < Item > {
        static void push(lua_State *l, const Item& item);
        static Item check(lua_State *l, int index);
    };

    template<>
    struct trait<FormId> : trait_base < FormId > {

        static void make_metatable(lua_State *l) {
            lua_settop(l, 0);
            luaL_newmetatable(l, table_name());

            set_relational<std::equal_to<FormId>>(l, "__eq");
            set_relational<std::less<FormId>>(l, "__lt");
            set_relational<std::less_equal<FormId>>(l, "__le");
        }

        template<class ComparisonFunc>
        static void set_relational(lua_State *l, const char *metaname) {
            lua_pushstring(l, metaname);
            lua_pushcfunction(l, comparison_func<ComparisonFunc>);
            lua_settable(l, 1);  /* metatable.__index = array.get */
        }

        template<class ComparisonFunc>
        static int comparison_func(lua_State *l) {
            bool result = ComparisonFunc() (check(l, 1), check(l, 2));
            lua_pushboolean(l, result);
            return 1;
        }

        static void push(lua_State *l, FormId formid) {
            FormId *data = (FormId*)lua_newuserdata(l, sizeof FormId);
            if (data) {
                *data = formid;
            }
            luaL_getmetatable(l, table_name());
            lua_setmetatable(l, -2); // also pops metatable, so obj is on top
        }

        static FormId check(lua_State *l, int idx) {
            FormId *usr = (FormId*)luaL_checkudata(l, idx, table_name());
            luaL_argcheck(l, usr != NULL, 1, "form identifier expected");
            return *usr;
        }
    };

    template<>
    struct trait<object_base> : trait_base<object_base> {
        static void make_metatable(lua_State *l) {
            lua_settop(l, 0);
            luaL_newmetatable(l, table_name());

            /* now the stack has the metatable at index 1 and
            `array' at index 2 */
            lua_pushstring(l, "__index");
            lua_pushcfunction(l, __index);
            lua_settable(l, 1);  /* metatable.__index = array.get */
        }

        static void push(lua_State *l, object_base* obj) {
            if (obj) {
                object_base** ptr = (object_base**)lua_newuserdata(l, sizeof(object_base*));
                if (ptr) {
                    *ptr = obj;
                }

                luaL_getmetatable(l, table_name());
                lua_setmetatable(l, -2); // also pops metatable, so obj is on top
            }
            else {
                lua_pushnil(l);
            }
        }

        static object_base* check(lua_State *l, int idx) {
            object_base **usr = (object_base **)luaL_checkudata(l, idx, table_name());
            luaL_argcheck(l, usr != NULL, 1, "JC object expected");
            return *usr;
        }

        static int __index(lua_State *l) {
            object_base * obj = check(l, 1);
            luaL_argcheck(l, obj != NULL, 1, "JC object expected");

            switch (lua_type(l, 2)) {
                case LUA_TSTRING: {
                    const char *keyString = luaL_checkstring(l, 2);
                    luaL_argcheck(l, obj->as<map>(), 1, "JMap expected");

                    trait<Item>::push(l, obj->as<map>()->find(keyString));
                    break;
                }

                case LUA_TNUMBER:  /* numbers */

                    if (obj->as<array>()) {
                        auto arr = obj->as<array>();
                        size_t index = luaL_checkunsigned(l, 2);
                        luaL_argcheck(l, index < arr->s_count(), 2, "index is out of JArray bounds");

                        trait<Item>::push(l, arr->getItem(index));
                    }
                    else if (obj->as<form_map>()) {
                        auto arr = obj->as<form_map>();
                        FormId formId = (FormId)lua_tounsigned(l, 2);
                        trait<Item>::push(l, arr->find(formId));
                    }
                    else {
                        luaL_argcheck(l, false, 1, "JArray or JFormMap containers expected");
                    }
                    break;

                case LUA_TUSERDATA: {
                    FormId fId = trait<FormId>::check(l, -1);
                    luaL_argcheck(l, obj->as<form_map>(), 1, "only JFormMap accepts form-id key");
                    trait<Item>::push(l, obj->as<form_map>()->find(fId));
                    break;
                }
                default:
                    luaL_argcheck(l, false, 2, "expected a number, string or form as a key");
                    break;
            }

            return 1;
        }
    };

    void trait<Item>::push(lua_State *l, const Item& item) {

        struct t : public boost::static_visitor<> {
            lua_State *l;

            void operator ()(const std::string& str) {
                lua_pushstring(l, str.c_str());
            }

            void operator ()(const Float32& val) {
                lua_pushnumber(l, val);
            }

            void operator ()(const boost::blank&) {
                lua_pushnil(l);
            }

            void operator ()(const collections::internal_object_ref& val) {
                trait<object_base>::push(l, val.get());
            }

            void operator ()(const FormId& val) {
                trait<FormId>::push(l, val);
            }

        } pusher;

        pusher.l = l;

        boost::apply_visitor(pusher, item.var());
    }

    Item trait<Item>::check(lua_State *l, int index) {
        if (lua_gettop(l) == 0) {
            return Item();
        }

        int type = lua_type(l, index);
        Item item;
        switch (type)
        {
        case LUA_TNUMBER:
            item = lua_tonumber(l, index);
            break;
        case LUA_TUSERDATA: {
            if (object_base *obj = trait<object_base>::check(l, index)) {
                item = obj;
            }
            else if (auto form = trait<FormId>::check(l, index)) {
                item = form;
            }
            else {
                assert(false);
            }
            break;
        }
        case LUA_TSTRING:
            item = luaL_checkstring(l, index);
            break;
        case LUA_TBOOLEAN:
            item = (int)lua_toboolean(l, index);
            break;
        default:
            break;
        }

        return item;
    }


    void stackDump(lua_State *l) {}

    template<class K, class V>
    struct trait<std::pair<K, V > > {

        typedef std::pair<K, V> self_type;

        typedef typename std::remove_const<K>::type first;

        static const char *table_name() {
            return typeid(self_type).name();
        }

        static void make_metatable(lua_State *l) {
        }

/*
        static self_type * check(lua_State *l, int idx) {
            void *usr = luaL_checkudata(l, idx, table_name());
            return (self_type *)usr;
        }
*/

        static void push(lua_State *l, const self_type& pair) {
            lua_createtable(l, 2, 0);

            lua_pushstring(l, "key");
            trait<first>::push(l, pair.first);
            lua_settable(l, -3);

            lua_pushstring(l, "value");
            trait<V>::push(l, pair.second);
            lua_settable(l, -3);
        }
    };
}
}

// group of apply-functions:
namespace collections {

    namespace {

        void printLuaError(lua_State *l) {
            if (lua_gettop(l) > 0 && lua_isstring(l, -1)) {
                const char *str = lua_tostring(l, -1);
                _MESSAGE("lua error: %s", str ? str : "");
            }
        }

        const char * object_to_apply_key = "jobject";

    // lua functions:

        object_base *get_object_to_apply(lua_State *l) {
            lua_getglobal(l, object_to_apply_key);
            auto obj = trait<object_base>::check(l, -1);
            if (obj) {
                lua_pop(l, 1);
            }
            return obj;
        }

        void set_object_to_apply(lua_State *l, object_base *obj) {
            trait<object_base>::push(l, obj);
            lua_setglobal(l, object_to_apply_key); // also pops var
        }

        template<class T, class F>
        static void apply_helper(lua_State *l, T *container, F& functor) {
            enum {
                none,
                path,
                predicate,
            };

            assert(container);
            auto container_copy = container->container_copy();

            const int top = lua_gettop(l);

            for (const auto& itm : container_copy) {

                lua_pushvalue(l, predicate);
                trait< std::remove_const<std::remove_reference<decltype(itm)>::type>::type >::push(l, itm);

                if (lua_pcall(l, 1, 1, 0) != 0) {
                    printLuaError(l);
                    break;
                }
                //-1 - shouldstop
                bool lambdaResult = lua_toboolean(l, -1) != 0;
                lua_settop(l, top);

                bool shouldStop = functor(itm, lambdaResult);

                if (shouldStop) {
                    break;
                }
            }
        }

        template< class F>
        static void apply(lua_State *l, F& functor) {

            // object, functor
            auto object = trait<object_base>::check(l, 1);
            luaL_argcheck(l, object, 1, "container expected");
            luaL_argcheck(l, lua_isfunction(l, 2), 2, "function expected");
            
            assert(object);

            stackDump(l);

            if (object->as<array>()) {
                apply_helper(l, object->as<array>(), functor);
            }
            else if (object->as<map>()) {
                apply_helper(l, object->as<map>(), functor);
            }
            else if (object->as<form_map>()) {
                apply_helper(l, object->as<form_map>(), functor);
            }
            else {
                assert(false);
            }

            stackDump(l);
        }

        static int lua_filter_func(lua_State *l) {
            // TODO: lua may panic at any time -> array memory will leak =\

            auto object = trait<object_base>::check(l, 1)->as<array>();
            luaL_argcheck(l, object != nullptr, 1, "JArray expected");
            luaL_argcheck(l, lua_isfunction(l, 2), 2, "function expected");

            array *obj = array::object(tes_context::instance());

            auto func = [=](const Item& item, bool predicateResult) {
                if (predicateResult) {
                    obj->push(item);
                }
                return false;
            };

            apply_helper(l, object, func);

            trait<object_base>::push(l, obj);

            return 1;
        }

        struct lua_apply_func_functor{
            template<class T> bool operator () (const T&, bool lResult) const {
                return lResult;
            }
        };

        static int lua_apply_func(lua_State *l) {

            apply(l, lua_apply_func_functor());

            return 0;
        }

    }

    namespace {

        void register_cfunctions(lua_State *l) {
            lua_settop(l, 0);

            lua_getglobal(l, "jc");

            // global 'jc' is loaded
            assert(lua_istable(l, -1));

            lua_pushstring(l, "apply");
            lua_pushcfunction(l, lua_apply_func);
            lua_settable(l, 1);

            lua_pushstring(l, "filter");
            lua_pushcfunction(l, lua_filter_func);
            lua_settable(l, 1);
        }

        void load_lua_code(lua_State *l) {

            namespace fs = boost::filesystem;

            auto do_string = [l](const char *code) {
                int failed = luaL_dostring(l, code);
                if (failed) {
                    printLuaError(l);
                }
            };

            auto loadFromFolder = [&](const char *folder) {

                fs::path dirPath = util::relative_to_dll_path(folder);

                if (!fs::is_directory(dirPath)) {
                    return;
                }

                char luaCode[1024] = { 0 };

                sprintf_s(luaCode, STR(
                    package.path = package.path .. ";%s?.lua"
                    ),
                    dirPath.generic_string().c_str());

                do_string(luaCode);

                fs::directory_iterator itr(dirPath), end;
                for (; itr != end; ++itr) {
                    const auto& path = itr->path();

                    if (!fs::is_regular_file(path) || fs::extension(path) != ".lua") {
                        continue;
                    }
                    
                    sprintf_s(luaCode, STR(require '%s'), path.filename().replace_extension().generic_string().c_str());
                    do_string(luaCode);
                }
            };

            loadFromFolder("JCData/lua/");
        }

        void load_traits(lua_State *l) {
            trait<object_base>::make_metatable(l);
            trait<std::pair<std::string, Item>>::make_metatable(l);
            trait<std::pair<FormId, Item>>::make_metatable(l);
            trait<FormId>::make_metatable(l);
        }

        void register_apply_functions(lua_State *l) {
            load_traits(l);
            load_lua_code(l);
            register_cfunctions(l);
        }
        LUA_CONTEXT_MODIFIER(register_apply_functions);
    }



    TEST(apply, t)
    {
        array *obj = array::object(tes_context::instance());
        obj->u_push(Item(1));
        obj->u_push(Item(2));
        obj->u_push(Item(3));

        map *root = map::object(tes_context::instance());
        root->setValueForKey("aKey", Item(obj));

        auto result = process_apply_func(obj, STR(
            return jc.find(jobject, jc.greater(2))
            ));

        EXPECT_TRUE(result.intValue() == 2);

        result = process_apply_func(root, STR(
            return jc.find(jobject.aKey, jc.greater(1))
            ));
        
        EXPECT_TRUE(result.intValue() == 1);

        result = process_apply_func(root, STR(
            return jc.count(jobject.aKey, jc.greater(1))
            ));

        EXPECT_TRUE(result.intValue() == 2);
    }

    TEST(apply2, t)
    {
        auto obj = json_deserializer::object_from_json_data(tes_context::instance(), STR(
            [
                {   "theSearchString": "a",
                    "theSearchForm" : "__formData|A|0x14"
                },

                {   "theSearchString": "b",
                    "theSearchForm" : "__formData|A|0x15"
                }
            ]
        ));

        auto result = process_apply_func(obj, STR(
            return jc.find(jobject, function(x) return x.theSearchString == 'b' end)
            ));

        EXPECT_TRUE(result.intValue() == 1);
    }

}
#endif
