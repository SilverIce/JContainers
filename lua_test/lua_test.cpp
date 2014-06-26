// lua_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <lua.hpp>

#include <vector>
#include <assert.h>

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
};

static const char __jrootObject;
static const char *kJContainersObject = "JContainers.Object";

void stackDump (lua_State *L);

void push_jobject(lua_State *L, object *obj) {
    stackDump(L);

    lua_pushlightuserdata(L, obj);
    luaL_getmetatable(L, kJContainersObject);
    int res = lua_setmetatable(L, -2); // also pops metatable, so obj is on top
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

object* check_jobject(lua_State *L) {
    stackDump(L);
    void *usr = luaL_checkudata(L, -1, kJContainersObject);
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
    lua_pushlightuserdata(L, (void *)&__jrootObject);  /* push address */
    lua_gettable(L, LUA_REGISTRYINDEX);  /* retrieve value */
    object *obj = check_jobject(L);
    lua_pop(L, 1);
    stackDump(L);
    return obj;
}


int lua_apply(lua_State *l) {

    enum  {
        func = 1,
        lambda,
        state,

    };

    // path, functor
    const char *path = luaL_checkstring(l, 1); // 1
   // auto func = lua_tocfunction(l, 2); // 2

    auto root = get_root_jobject(l);

    lua_pushnil(l); // 3. state

    stackDump(l);

    for (const auto& itm : *root) {

        lua_pushvalue(l, lambda);//4 func
        push_item(l, itm);//5 x
        lua_pushvalue(l, state);//6 state

        if (lua_pcall(l, 2, 2, 0) != 0) {
           ;// error(L, "error running function `f': %s", lua_tostring(L, -1));
        }
        //-1 - state
        //-2 - shouldstop

        int shouldStop = lua_toboolean(l, -2);

        lua_insert(l, state); // move top into state.3
        lua_settop(l, state);

        stackDump(l);

        if (shouldStop) {
            break;
        }
    }

    lua_insert(l, 1);
    lua_settop(l, 1);

    stackDump(l);

    //lua_pushstring(l, "wow what a magick!");

    return 1;
}

static void stackDump (lua_State *L) {
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

    int res = luaL_dostring(l, STR(
        return apply(
            '.path',
            function(x, state)
                if x == 4 then return true,x else return false,nil end
            end
        )
            
        ));

    stackDump(l);
}

int _tmain(int argc, _TCHAR* argv[])
{
    auto l = luaL_newstate();

    luaL_newmetatable(l, kJContainersObject);
    lua_pop(l, 1);

    lua_pushcfunction(l, lua_apply);
    lua_setglobal(l, "apply");
    stackDump(l);


    object ar;
    item itms[] = {
        item_number, 0,
        item_number, 1,
        item_number, 2,
        item_number, 3,
        item_number, 4,
        item_number, 5,
    };

    for (auto& it : itms) {
        ar.push_back(it);
    }
    
    apply_call(&ar, l, "");


    //int res = luaL_dostring(l, "return apply( '.path', function(x) return 10 end )");

    stackDump(l);

    lua_close(l);

	return 0;
}

