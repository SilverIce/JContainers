#include "collections\access.h"
namespace tes_api_3 {

    using namespace collections;

#if 0
    class tes_lua : public class_meta < tes_lua > {
    public:

        void additionalSetup() {
            metaInfo.comment = "executes lua code";
        }

        /*
        
        popTransport db = pop db["__luaTransportPool"]

        retrieveOrMake tree key def = tree[key]

        pop (x:xs) = x
        pop [] = Transport
        
        */

        static array& get_transport_pool(tes_context& ctx) {

            auto root = ctx.database();

            object_lock l(root);

            auto itm = root->u_get("__luaTransportPool");
            if (!itm) {
                itm = root->u_set("__luaTransportPool", array::object(ctx));
            }

            return itm->object()->as_link<array>();
        }

        static array* transport() {
            array* transport = nullptr;
            {
                auto& ctx = tes_context::instance();
                array& transportPool = get_transport_pool(ctx);

                object_lock l(transportPool);
                auto var = transportPool.u_erase_and_return(-1);
                if (var) {
                    transport = var->object()->as<array>();
                } else {
                    transport = &array::object(ctx);
                }
            }
            return transport;
        }

        static int evalLuaInt(const char* luaCode, array* transport) {
            auto result = lua::eval_lua_function(tes_context::instance(), obj, luaCode);
            return result ? result->readAs<T>() : def;
        }


        REGISTER_TES_NAME("JCLua");

        //evalLuaInt("return a[1] + a[2]", pushInt(1, pushInt(2, LuaTransport())))
    };

    TES_META_INFO(tes_lua);
#endif
}
 