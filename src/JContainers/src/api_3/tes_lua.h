
namespace tes_api_3 {

    using namespace collections;

#if 1
    class tes_lua : public class_meta < tes_lua > {
    public:

        void additionalSetup() {
            metaInfo.comment = "Evaluates Lua code. Unstable API - I'm free to change or remove it anytime";
        }

#define ARGNAMES "luaCode transport default="
#define ARGNAMES_2 " minimizeLifetime=true"
        REGISTERF(evalLua<Float32>, "evalLuaFlt", ARGNAMES "0.0" ARGNAMES_2,
R"===(Evaluates piece of Lua code. The arguments are carried by @transport object.
The @transport is any kind of object, not just JMap.
If @minimizeLifetime is True the function will invoke JValue.zeroLifetime on the @transport object.
It is more than wise to re-use @transport when evaluating lot of lua code at once.
Returns @default value if evaluation fails.
Usage example:

    ; 7 from the end until 9 from the end. Returns "Lua" string
    string input = "Hello Lua user"
    string s = JLua.evaLuaStr("return string.sub(args.string, args.low, args.high)",\
        JLua.setStr("string",input, JLua.setInt("low",7, JLua.setInt("high",9 )))\
    )
)===");
        REGISTERF(evalLua<SInt32>, "evalLuaInt", ARGNAMES "0" ARGNAMES_2, nullptr);
        REGISTERF(evalLua<skse::string_ref>, "evalLuaStr", ARGNAMES R"("")" ARGNAMES_2, nullptr);
        REGISTERF(evalLua<Handle>, "evalLuaObj", ARGNAMES "0" ARGNAMES_2, nullptr);
        REGISTERF(evalLua<TESForm*>, "evalLuaForm", ARGNAMES "None" ARGNAMES_2, nullptr);
#undef ARGNAMES
#undef ARGNAMES_2

        template<class ResultType>
        static ResultType evalLua(tes_context& ctx, const char* luaCode, object_base* transport, ResultType def, bool minimizeLifetime = true) {
            auto result = lua::eval_lua_function(ctx, transport, luaCode);
            if (transport && minimizeLifetime) {
                transport->zero_lifetime();
            }
            return result ? result->readAs<ResultType>() : def;
        }

#define ARGNAMES "key value transport=0"
        REGISTERF(pushArg<const char*>, "setStr", ARGNAMES,
R"===(Inserts new (or replaces existing) {key -> value} pair. Expects that @transport is JMap object, if @transport is 0 it creates new JMap object.
Returns @transport)===");
        REGISTERF(pushArg<Float32>, "setFlt", ARGNAMES, "");
        REGISTERF(pushArg<SInt32>, "setInt", ARGNAMES, "");
        REGISTERF(pushArg<form_ref>, "setForm", ARGNAMES, "");
        REGISTERF(pushArg<object_base*>, "setObj", ARGNAMES, "");
#undef ARGNAMES

        template<class ArgType>
        static map* pushArg(tes_context& ctx, const char* key, ArgType arg, map* transport = nullptr) {
            if (!transport) {
                transport = &map::object(ctx);
            }

            tes_map::setItem<ArgType>(ctx, transport, key, arg);
            return transport;
        }

        REGISTER_TES_NAME("JLua");
    };

    TEST(JLua, simple)
    {
        tes_context_standalone ctx;

        EXPECT_EQ(8, tes_lua::evalLua<float>(ctx, "return args.x * args.y", tes_lua::pushArg(ctx, "x", 2, tes_lua::pushArg(ctx, "y", 4)), 0.f));

        EXPECT_EQ(1, tes_lua::evalLua<SInt32>(ctx, "return jobject ~= nil", tes_lua::pushArg(ctx, "garbage", 4), -1))
            << "@jobject (@args alias) isn't supported";

        EXPECT_EQ (0, tes_lua::evalLua<SInt32> (ctx, "return bit.bxor (8, 2, 10)", nullptr, -1));
    }

    TES_META_INFO(tes_lua);
#endif
}
 