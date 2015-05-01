
#include "lua_module.h"

#include <boost/noncopyable.hpp>
#include <boost/thread/tss.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lockfree/queue.hpp>

#include <utility>
#include <string>
#include <sstream>
#include <mutex>
#include <algorithm>
#include <thread>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "meta.h"
#include "util/util.h"
#include "gtest.h"
#include "util/spinlock.h"

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

namespace lua { namespace aux_wip {

    using namespace api;

    enum  {
        LUA_OK = 0,
    };

    class context : public boost::noncopyable {

        lua_State *l = nullptr;
        tes_context& tcontext;

    public:

        lua_State *state() const {
            return l;
        }

        explicit context(tes_context& context) : tcontext(context) {
            reopen_if_closed();
            _DMESSAGE("Lua context created");
        }

        ~context() {
            close();
            _DMESSAGE("Lua context destructed");
        }

        boost::optional<Item> eval_lua_function(object_base *object, const char *lua_string) {

            assert(lua_string);

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

    private:

        static const char* top_string(lua_State *l) {
            if (lua_gettop(l) > 0 && lua_isstring(l, -1)) {
                return lua_tostring(l, -1);
            }
            return "<can't obtain string>";
        }

        static void print_top_string(lua_State *l, const char* preambula) {
            _MESSAGE("%s: %s", preambula, top_string(l));
            //skse::console_print(str);
        }

        static bool setupLuaContext(lua_State *l, tes_context& context) {
            typedef boost::filesystem::path path;

            auto initScriptPath = util::relative_to_dll_path(JC_DATA_FILES "InternalLuaScripts/init.lua");

            lua_pushcfunction(l, LuaErrorHandler);
            int errorHandler = lua_gettop(l);

            if (luaL_loadfile(l, initScriptPath.generic_string().c_str()) != LUA_OK) {
                print_top_string(l, "Lua unable to load init.lua and etc");
                return false;
            }

            // push data path, dll path
            lua_pushstring(l, util::relative_to_dll_path(JC_DATA_FILES).generic_string().c_str());
            lua_pushstring(l, util::dll_path().generic_string().c_str());
            lua_pushlightuserdata(l, &context);
            enum { num_lua_args = 3 }; // DONT FORGET ME!!

            if (lua_pcall(l, num_lua_args, LUA_MULTRET, errorHandler) != LUA_OK) {
                print_top_string(l, "Lua unable to execute init.lua and etc");
                return false;
            }

            return true;
        }

        static int LuaErrorHandler(lua_State *l) {

            int message = lua_gettop(l);//1
            print_top_string(l, "Lua error");

            lua_getglobal(l, "debug");//2
            lua_pushstring(l, "traceback");
            lua_gettable(l, 2);
            //int function = lua_gettop(l);

            lua_call(l, 0, 1);

            print_top_string(l, "Lua trace");

            lua_pushvalue(l, message);
            return 1;
        }
    };

    // just a pool, factory of contexts.
    // any thead can obtain free (or newly created), initialized lua-context
    // the tread have to return it back via @release
    class context_pool : public collections::dependent_context, boost::noncopyable {
        static const uint32_t queue_capacity = 16;

        boost::lockfree::queue<context*> _queue = queue_capacity;
        tes_context& _tcontext;
        std::atomic_int32_t _aquired_count = 0;

    public:

        context& aquire() {
            ++_aquired_count;

            context *ctx = nullptr;

            if (!_queue.pop(ctx)) {
                ctx = new context(_tcontext);
            }

            assert(ctx);
            return *ctx;
        }

        void release(context& ctx) {
            --_aquired_count;

            _queue.push(&ctx);
        }

        explicit context_pool(tes_context& tc)
            : _tcontext(tc)
        {
            tc.add_dependent_context(*this);
        }

        ~context_pool() {
            _tcontext.remove_dependent_context(*this);
            clear();
        }

        void clear_state() override {
            clear();
        }

    private:

        void clear() {
            warn_if_aquired();
            _queue.consume_all([](context *ctx) {
                assert(ctx);
                delete ctx;
            });
        }

        void warn_if_aquired() {
            if (_aquired_count > 0) { // ololo. N contexts are still aquired, crash possible!
                assert(false);
            }
        }
    };

    // aquires a context from context_pool and releases it when destroyed
    class autofreed_context : boost::noncopyable {
        context_pool& _pool;
        context& _context;
    public:
        explicit autofreed_context(context_pool& pool)
            : _pool(pool)
            , _context(pool.aquire())
        {}

        ~autofreed_context() {
            _pool.release(_context);
        }

        context* operator -> () { return &_context; }
        context& context() { return _context; }
    };


    struct fixture : testing::Fixture {
        tes_context tc;
        context_pool pool;
        fixture() : pool(tc) {}
    };

    TEST_F(fixture, Lua, trtr)
    {
        std::atomic_int8_t stop = 0;

        auto task = [&]() {
            while (stop.load(std::memory_order_acquire) == 0) {
                EXPECT_TRUE(autofreed_context(pool)->eval_lua_function(nullptr, "return testing.perform()")->intValue() != 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        };

        std::thread t1(task), t2(task);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        stop.store(1, std::memory_order_release);

        t1.join();
        t2.join();
    }

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

        autofreed_context lc(pool);

        auto testTransporting = [&](const char *str) { return lc->eval_lua_function(nullptr, str); };

        EXPECT_TRUE(*testTransporting("return 10") == 10.0f);
        EXPECT_TRUE(*testTransporting("return 'die'") == std::string("die"));
        EXPECT_TRUE(*testTransporting("return Form(20)") == FormId(20));


        auto& db = *tc.database();
        EXPECT_TRUE(*testTransporting("return JDB") == db.base());

        db.setValueForKey("test", 10);
        EXPECT_TRUE(db["test"] == 10);

        auto result = lc->eval_lua_function(nullptr,
            R"===(for k,v in pairs(JDB) do print('JDB.test', k, v) end;
            assert(JDB.test == 10);
            return JDB.test)==="
            );
        EXPECT_TRUE(result.is_initialized());
        EXPECT_TRUE(*result == 10.f);

    }

    TEST_F(fixture, Lua, launch_all_lua_tests)
    {
        EXPECT_TRUE(autofreed_context(pool)->eval_lua_function(nullptr, "return testing.perform()")->intValue() != 0);
    }

}
}

namespace lua {

    boost::optional<Item> eval_lua_function(tes_context& ctx, object_base *object, const char *lua_string) {
        auto pool = static_cast<aux_wip::context_pool*>(*ctx.lua_context);
        return aux_wip::autofreed_context(*pool)->eval_lua_function(object, lua_string);
    }

    static tes_context::post_init g_extender([](tes_context& ctx){
        ctx.lua_context = std::make_unique<collections::dependent_context*>(new aux_wip::context_pool(ctx));
    });
}
