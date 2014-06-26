// lua_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <lua.hpp>

 void stackDump (lua_State *L);

int lua_apply(lua_State *L) {
    // path, functor
    const char *path = luaL_checkstring(L, 1);
    auto func = lua_tocfunction(L, 2);

    lua_pushstring(L, "wow what a magick!");

    stackDump(L);

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

int _tmain(int argc, _TCHAR* argv[])
{
    auto l = luaL_newstate();

    lua_pushcfunction(l, lua_apply);
    lua_setglobal(l, "apply");

    int res = luaL_dostring(l, "return apply( '.path', function(x) return 10 end )");

    stackDump(l);

    lua_close(l);

	return 0;
}

