
#include "lua_module.h"

#include <boost/noncopyable.hpp>
#include <boost/thread/tss.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem/path.hpp>

#include <utility>
#include <string>
#include <sstream>
#include <mutex>
#include <algorithm>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "meta.h"
#include "util.h"
#include "gtest.h"
#include "spinlock.h"

#include "reflection.h"
#include "jcontainers_constants.h"
#include "collections.h"
#include "tes_context.h"
#include "collection_functions.h"
#include "path_resolving.h"

// Module imports:

namespace lua {
    using tes_context = collections::tes_context;

    using object_base = collections::object_base;

    using Item = collections::Item;
    using item_type = collections::item_type;
    using FormId = collections::FormId;
}

#include "lua_native_funcs.hpp"

namespace lua { namespace aux {

    enum  {
        LUA_OK = 0,
    };

    class context : public collections::dependent_context, boost::noncopyable {

        lua_State *l = nullptr;
        tes_context& tcontext;

    public:

        lua_State *state() const {
            return l;
        }

        explicit context(tes_context& context) : tcontext(context) {
            reopen_if_closed();
            context.add_dependent_context(*this);
            _DMESSAGE("Lua context created");
        }

        ~context() {
            tcontext.remove_dependent_context(*this);
            close();
            _DMESSAGE("Lua context destructed");
        }

        boost::optional<Item> eval_lua_function(object_base *object, const char *lua_string) {

            assert(lua_string);
            assert(!object || &object->context() == &tcontext);

            lua_pushcfunction(l, LuaErrorHandler);
            int errorHandler = lua_gettop(l);

            lua_getglobal(l, "JC_compileAndRun");
            lua_pushstring(l, lua_string);
            lua_pushlightuserdata(l, object);
            enum { num_args = 2, returned = 1 };

            if (lua_pcall(l, num_args, returned, errorHandler) != LUA_OK) {
                return boost::none;
            }
            else {
                Item result;
                JCValue *val = (JCValue *)lua_topointer(l, -1);
                JCValue_fillItem(val, result);
                return result;
            }
        }

        void reopen_if_closed() {
            if (!l) {
                l = luaL_newstate();
                luaL_openlibs(l);
                setupLuaContext(l, tcontext);
            }
        }

        void close() {
            if (l) {
                auto lua = l;
                l = nullptr;
                lua_close(lua);
            }
        }

        void clear_state() override {
            close();
            _DMESSAGE("Lua context cleaned from clear_state");
        }

    private:

        static void printLuaError(lua_State *l) {
            if (lua_gettop(l) > 0 && lua_isstring(l, -1)) {
                const char *str = lua_tostring(l, -1);
                _MESSAGE("Lua error: %s", str ? str : "");
                //skse::console_print(str);
            }
        }

        static bool setupLuaContext(lua_State *l, tes_context& context) {
            typedef boost::filesystem::path path;

            auto initScriptPath = util::relative_to_dll_path(JC_DATA_FILES "InternalLuaScripts/init.lua");

            lua_pushcfunction(l, LuaErrorHandler);
            int errorHandler = lua_gettop(l);

            if (luaL_loadfile(l, initScriptPath.generic_string().c_str()) != LUA_OK) {
                printLuaError(l);
                return false;
            }

            // push data path, dll path
            lua_pushstring(l, util::relative_to_dll_path(JC_DATA_FILES).generic_string().c_str());
            lua_pushstring(l, util::dll_path().generic_string().c_str());
            lua_pushlightuserdata(l, &context);
            enum { num_lua_args = 3 }; // DONT FORGET ME!!

            if (lua_pcall(l, num_lua_args, LUA_MULTRET, errorHandler) != LUA_OK) {
                printLuaError(l);
                return false;
            }

            return true;
        }

        static int LuaErrorHandler(lua_State *l) {

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
    };

    // For production, non-test use only!!! - as it points to a single tes_context::instance()
    class threadlocal_context : boost::noncopyable {

        static boost::thread_specific_ptr<context> __state;

    public:

        // thread-local single instance
        static context& instance(){
            context * ctx = __state.get();
            if (!ctx) {
                __state.reset( ctx = new context(tes_context::instance()) );
            }
            ctx->reopen_if_closed();
            return *ctx;
        }


        // Will be called when thread-local @__state will be destructed (at thread destruction)
        static void destructor(context *c) {
            jc_debug("Lua: threadlocal_context dtor")
            delete c;
        }
    };

    boost::thread_specific_ptr<context> threadlocal_context::__state(&threadlocal_context::destructor);

    struct fixture : testing::Fixture {
        tes_context tc;
        context lc;
        fixture() : lc(tc) {}
        ~fixture() {
            lc.close();
        }
    };

    TEST_F(fixture, Lua, evalLua)
    {
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

        auto testTransporting = [&](const char *str) { return lc.eval_lua_function(nullptr, str); };

        EXPECT_TRUE(testTransporting("return 10")->intValue() == 10);
        EXPECT_TRUE(testTransporting("return 'die'")->strValue() == std::string("die"));
        EXPECT_TRUE(testTransporting("return Form(20)")->formId() == FormId(20));


        auto db = tc.database();
        EXPECT_TRUE(testTransporting("return JDB")->object() == db);

        db->setValueForKey("test", Item(10));
        EXPECT_TRUE(db->find("test").intValue() == 10);

        auto result = lc.eval_lua_function(nullptr,
            R"===(for k,v in pairs(JDB) do print('JDB.test', k, v) end;
                assert(JDB.test == 10);
                return JDB.test)==="
                );
        EXPECT_TRUE(result.is_initialized());
        EXPECT_TRUE(result->intValue() == 10);

    }

    TEST_F(fixture, Lua, launch_all_lua_tests)
    {
        EXPECT_TRUE(lc.eval_lua_function(nullptr, "return testing.perform()")->intValue() != 0);
    }


}
}

namespace lua {

    boost::optional<Item> eval_lua_function(object_base *object, const char *lua_string) {
        return aux::threadlocal_context::instance().eval_lua_function(object, lua_string);
    }

}
