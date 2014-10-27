#pragma once

#include <boost/noncopyable.hpp>
#include <mutex>
#include <algorithm>

extern "C" {

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

}

#include "meta.h"
#include "gtest.h"
#include "spinlock.h"
//#include "util.h"
//#include <boost/filesystem/path.hpp>

// Basic Lua-related-only things
namespace lua {

    enum  {
        LUA_OK = 0,
    };

    // Caches a strings compiled into a lua-functions:
    // Stores associations {luaString: compiledFunction} in a weak lua table

    struct fixture : testing::Fixture {
        lua_State *l;
        fixture() : l(luaL_newstate()) { luaL_openlibs(l); }
        ~fixture() { lua_close(l); l = nullptr; }
    };


/*
    namespace function_cache {

        static char jc_compile_function_key;

        void setup(lua_State *L) {

            assert(LUA_OK == luaL_dostring(L, STR(
                local jc_function_cache = {}
                setmetatable(jc_function_cache, { __mode = 'v' })

                local jc_compile = function(luaString)
                    print('jc_compile', luaString)
                    local fc = jc_function_cache
                    local funcOrError = fc[luaString]
                    if not funcOrError then
                        local f, errorMsg = loadstring(luaString)
                        funcOrError = (f or errorMsg)
                        fc[luaString] = funcOrError
                    end

                    return funcOrError
                end

                return jc_compile
            )));

            int functionIdx = lua_gettop(L);
            assert(lua_isfunction(L, functionIdx));
 
            lua_pushlightuserdata(L, &jc_compile_function_key); // push key
            lua_pushvalue(L, functionIdx);
            lua_settable(L, LUA_REGISTRYINDEX);

            lua_settop(L, 0);
        }

        // pushes compiled function on stack in case of success or error string in case of failure
        // returns true
        bool compile(lua_State *l, const char *lua_string) {
            assert(lua_string);

            lua_pushlightuserdata(l, &jc_compile_function_key); // push key
            lua_gettable(l, LUA_REGISTRYINDEX); // get 'jc_compile' function
            assert(lua_isfunction(l, -1));
            lua_pushstring(l, lua_string);
            // invoke 'jc_compile' function
            return lua_pcall(l, 1, 1, 0) == LUA_OK;
        }

        namespace {
            // anonymous namespace to not pollute other namespaces

            TEST_F(fixture, _, function_cache)
            {
                setup(l);

                EXPECT_TRUE(compile(l, STR(
                    return 'message' .. 's'
                )));
                
                EXPECT_TRUE( lua_isfunction(l, -1) );

                lua_pushinteger(l, 4);
                lua_pushinteger(l, 1);
                EXPECT_TRUE( lua_pcall(l, 2, 1, 0) == LUA_OK );

                EXPECT_TRUE(strcmp(lua_tostring(l,-1), "messages") == 0);
            }

            TEST_F(fixture, _, ttt)
            {
                setup(l);

                assert(LUA_OK == luaL_dostring(l, STR(
                    local ffi = require("ffi")

                    ffi.cdef[[
                        typedef struct _T { int32_t x, y; } T;
                    ]]

                        local T = ffi.typeof('T')

                        return T(8, 9)

                )));

                    typedef struct _T { int x, y; } T;

                    T* t = (T*)lua_topointer(l, -1);
            }
        }
    };
*/


    struct context_modifier_tag {};

#   define LUA_CONTEXT_MODIFIER(function_modifier) \
        namespace { static const ::meta<void (*)(lua_State *), ::lua::context_modifier_tag> g_lua_modifier_##function_modifier(function_modifier); }

    class context : boost::noncopyable {

        static __declspec(thread) context * __state;

        lua_State *l;

    public:

        lua_State *state() const {
            return l;
        }

        // thread-local single instance
        static context& instance() {
            context * ctx = __state;
            if (!ctx) {
                __state = ctx = new context();
                add_context(*ctx);
            }
            return *ctx;
        }

        static void shutdown_all() {
            guard g(_contexts_lock);
            for (auto& c : _contexts) {
                c->close_lua();
            }
        }

        // Not threadsafe. Not supposed to be threadsafe!
        void close_lua() {
            if (l) {
                lua_close(l);
                l = nullptr;
            }
        }

    private:

        context()
            : l(luaL_newstate())
        {
            luaL_openlibs(l);

            //function_cache::setup(l);

            for (const auto& modifier : meta<void(*)(lua_State *), context_modifier_tag>::getListConst()) {
                modifier(l);
            }

        }

        ~context() {
            remove_context(*this);
            close_lua();
        }
        
    private:

        static std::vector<context *> _contexts;
        static collections::spinlock _contexts_lock;
        typedef std::lock_guard<collections::spinlock> guard;

        static void add_context(context &c) {
            guard g(_contexts_lock);
            if (std::find(_contexts.begin(), _contexts.end(), &c) == _contexts.end()) {
                _contexts.push_back(&c);
            }
        }

        static void remove_context(context &c) {
            guard g(_contexts_lock);
            _contexts.erase(std::remove(_contexts.begin(), _contexts.end(), &c), _contexts.end());
        }
    };

    void shutdown_all_contexts() {
        context::shutdown_all();
    }

    context* context::__state = nullptr;

    std::vector<context *> context::_contexts;
    collections::spinlock context::_contexts_lock;

}
