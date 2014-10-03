#pragma once

#include "lua_stuff.h"

#include <lua.hpp>
#include <utility>
#include <string>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>

#include "meta.h"
#include "gtest.h"

#include "jcontainers_constants.h"
#include "collections.h"
#include "json_handling.h"
#include "skse.h"
#include "tes_context.h"
#include "util.h"

namespace collections { namespace lua_traits {

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

            switch (lua_type(l, 2)){
                case LUA_TSTRING: {
                    const char *keyString = luaL_checkstring(l, 2);
                    luaL_argcheck(l, obj->as<map>(), 2, "JMap expected");

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
                    luaL_argcheck(l, false, 2, "JC object expects number or string key only");
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


namespace collections {

    struct lua_context_modifier_tag {};

#   define LUA_CONTEXT_MODIFIER(function_modifier) \
        static const ::meta<void (*)(lua_State *), collections::lua_context_modifier_tag> g_lua_modifier_##function_modifier(function_modifier);

    class lua_context : boost::noncopyable {

        static __declspec(thread) lua_context * __state;

        lua_State *l;

    public:

        lua_State *state() const {
            return l;
        }

        // thread-local single instance
        static lua_context& instance() {
            if (!__state) {
                __state = new lua_context();
            }
            return *__state;
        }

    private:

        lua_context()
            : l(luaL_newstate())
        {
            luaL_openlibs(l);
/*
            luaopen_base(l);
            luaopen_package(l);
            luaopen_math(l);
            luaopen_bit32(l);
            luaopen_string(l);*/

            using namespace std;
            using namespace lua_traits;

            trait<object_base>::make_metatable(l);
            trait<pair<string, Item>>::make_metatable(l);
            trait<pair<FormId, Item>>::make_metatable(l);
            trait<FormId>::make_metatable(l);

            for (const auto& modifier : meta<void (*)(lua_State *), collections::lua_context_modifier_tag>::getListConst()) {
                modifier(l);
            }
        }

        ~lua_context() {
            lua_close(l);
            l = nullptr;
        }
    };

    lua_context* lua_context::__state = nullptr;

}

// group of apply-functions:
namespace collections { namespace lua_apply {

    void printLuaError(lua_State *l) {
        if (lua_gettop(l) > 0 && lua_isstring(l, -1)) {
            const char *str = lua_tostring(l, -1);
            _MESSAGE("lua error: %s", str ? str : "");
        }
    }

    namespace {

        using namespace lua_traits;

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
                lua_traits::trait< std::remove_const<std::remove_reference<decltype(itm)>::type>::type >::push(l, itm);

                if (lua_pcall(l, 1, 1, 0) != 0) {
                    luaL_error(l, "error running function `f': %s", lua_tostring(l, -1));
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

        void register_apply_functions(lua_State *l) {
            load_lua_code(l);
            register_cfunctions(l);
        }
        LUA_CONTEXT_MODIFIER(register_apply_functions);
    }

    Item process_apply_func(object_base *object, const char *lua_string) {

        auto l = lua_context::instance().state();
        Item result;
        set_object_to_apply(l, object);
        {
            int error = luaL_dostring(l, lua_string);
            if (error) {
                printLuaError(l);
            }
            result = trait<Item>::check(l, -1);
        }
        set_object_to_apply(l, nullptr);

        return result;
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
}