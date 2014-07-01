// lua_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <lua.hpp>

#include <vector>
#include <assert.h>
#include <chrono>
#include <iostream>

enum item_type {
    item_none,
    item_object,
    item_number,
    item_string,
};

struct item;
typedef std::vector<item> object;

struct item {
    item_type type;

    union {
        double number;
        object *object;
        const char *string;
    };

    explicit item(double num) : type(item_number), number(num) {}
    explicit item(int num) : type(item_number), number(num) {}

    explicit item(std::vector<item> *obj) : type(item_object), object(obj) {}

    explicit item(const char *str) : type(item_string), string(str) {}

    item() : type(item_none) {}

};

static const char __jrootObject;
static const char *kJContainersObject = "JContainers.Object";

void stackDump (lua_State *L);

void push_jobject(lua_State *L, object *obj) {
    stackDump(L);

    lua_pushlightuserdata(L, obj);
    luaL_getmetatable(L, kJContainersObject);
    lua_setmetatable(L, -2); // also pops metatable, so obj is on top
    stackDump(L);

}

void push_item(lua_State *L, const item& itm) {
    switch (itm.type)
    {
    case item_none:
        lua_pushnil (L);
        break;
    case item_number:
        lua_pushnumber(L, itm.number);
        break;
    case item_object:
        push_jobject(L, itm.object);
        break;
    case item_string:
        lua_pushstring(L, itm.string);
        break;
    default:
        assert(false);
        break;
    }
}

object* check_jobject(lua_State *L, int idx) {
    stackDump(L);
    void *usr = luaL_checkudata(L, idx, kJContainersObject);
    luaL_argcheck(L, usr != NULL, 1, "JC object expected");
    stackDump(L);
    return (object *)usr;
}

void set_root_jobject(lua_State *L, object *obj) {
    stackDump(L);

    lua_pushlightuserdata(L, (void *)&__jrootObject);
    push_jobject(L, obj);
    lua_settable(L, LUA_REGISTRYINDEX);

    stackDump(L);
}

object* get_root_jobject(lua_State *L) {
    lua_pushlightuserdata(L, (void *)&__jrootObject);  /* push key */
    lua_gettable(L, LUA_REGISTRYINDEX);  /* retrieve value */
    object *obj = check_jobject(L, -1);
    lua_pop(L, 1);
    stackDump(L);
    return obj;
}


int lua_apply(lua_State *l) {

    enum  {
        none,
        path,
        lambda,
    };


    // path, functor
    //const char *path = luaL_checkstring(l, 1); // 1
   // auto func = lua_tocfunction(l, 2); // 2

    auto root = get_root_jobject(l);

    stackDump(l);

    for (const auto& itm : *root) {

        lua_pushvalue(l, lambda);//3 func
        push_item(l, itm);//4 x

        if (lua_pcall(l, 1, 1, 0) != 0) {
            luaL_error(l, "error running function `f': %s", lua_tostring(l, -1));
        }
        //-1 - shouldstop

        int shouldStop = lua_toboolean(l, -1);

        lua_settop(l, lambda);

        stackDump(l);

        if (shouldStop) {
            break;
        }
    }

   // lua_insert(l, 1);
   // lua_settop(l, 1);

    stackDump(l);

    return 0;
}

int jobject_get_value_for_key(lua_State *l) {
    object *obj = check_jobject(l, 1);// 1st

    luaL_argcheck(l, obj != nullptr, 1, "jobject is nil");

    int keyType = lua_type(l, 2);

    const char *keyString = nullptr;
    int intKey = 0;

    switch (keyType) {
    case LUA_TSTRING:  /* strings */
        assert(false);
        keyString = lua_tostring(l, 2);
        break;

    case LUA_TNUMBER:  /* numbers */
        intKey = lua_tointeger(l, 2);
        break;

    default:
        luaL_argcheck(l, false, 2, "invalid key");
        return 1;

    }

    push_item(l, (*obj)[intKey]);

    return 1;
}

static void stackDump (lua_State *L) {
    return ;
    int i;
    int top = lua_gettop(L);
    for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {

        case LUA_TSTRING:  /* strings */
            printf("`%s'", lua_tostring(L, i));
            break;

        case LUA_TBOOLEAN:  /* booleans */
            printf(lua_toboolean(L, i) ? "true" : "false");
            break;

        case LUA_TNUMBER:  /* numbers */
            printf("%g", lua_tonumber(L, i));
            break;

        default:  /* other values */
            printf("%s", lua_typename(L, t));
            break;

        }
        printf("  ");  /* put a separator */
    }
    printf("\n");  /* end the listing */
}

#define STR(...) #__VA_ARGS__

void apply_call(object *obj, lua_State *l, const char *lua_string) {
    set_root_jobject(l, obj);
    stackDump(l);

   // printf("lua string: %s\n", lua_string); 

    int res = luaL_dostring(l, lua_string);
    //assert(res == 0);

    stackDump(l);
}

void setup_lua(lua_State *l) {
    lua_settop(l, 0);
   // luaopen_io(l);

    luaL_newmetatable(l, kJContainersObject);
    assert(lua_istable(l, 1));

    /* now the stack has the metatable at index 1 and
    `array' at index 2 */
    lua_pushstring(l, "__index");
    lua_pushcfunction(l, jobject_get_value_for_key);
    lua_settable(l, 1);  /* metatable.__index = array.get */

    lua_pushcfunction(l, lua_apply);
    lua_setglobal(l, "apply");

}

int _tmain(int argc, _TCHAR* argv[])
{
    auto l = luaL_newstate();

    luaopen_base(l);

    setup_lua(l);

    luaL_dostring(l, STR(
        function filter(path, predicate)
            local state = {}

            apply(path,
                function(x)
                    if predicate(x) then state[#state + 1] = x end
                    return false
                end
            )

            return state
        end
        ));

    luaL_dostring(l, STR(
        function find(path, predicate)
            local foundValue

            apply(path,
                function(x)
                    if predicate(x) then
                        foundValue = x
                        return true
                    else
                        return false
                    end
                end
            )

            return foundValue
        end
        ));



    object ar;

    object ar2;
    ar2.push_back(item(3));
    ar2.push_back(item(2));

    item itms[] = {
/*
        item(0),
        item(1),
        item(4),
*/
        item(&ar2),
        item(&ar2),
    };

    for (auto& it : itms) {
        ar.push_back(it);
    }

    auto start = std::chrono::system_clock::now();

    for (int i = 2000 - 1; i >= 0 ; i--)
    {
        apply_call(&ar, l, STR(
            return find(
                '.path',
                function(x)
                    return x[1] < 4
                end
                )
            ));

    }
    

    auto end = std::chrono::system_clock::now();


    std::cout << "diff " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;
    //int res = luaL_dostring(l, "return apply( '.path', function(x) return 10 end )");

    stackDump(l);

    lua_close(l);

	return 0;
}

