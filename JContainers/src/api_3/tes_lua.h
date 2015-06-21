namespace collections {
#if 0
    class tes_lua : public class_meta < tes_lua > {
    public:

        void additionalSetup() {
            metaInfo.comment = "executes lua code";
        }

        REGISTER_TES_NAME("JCLua");

        evalLuaInt("return a[1] + a[2]", pushInt(1, pushInt(2, LuaTransport())))
    };

    TES_META_INFO(tes_lua);
#endif
}
 