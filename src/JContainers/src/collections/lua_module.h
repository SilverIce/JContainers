#pragma once

namespace collections {
    class object_base;
    class item;
    class dependent_context;
    class tes_context;
}

namespace boost {
    template<class T> class optional;
}

namespace lua {

    boost::optional<collections::item> eval_lua_function(   collections::tes_context& ctx,
                                                            collections::object_base *object,
                                                            const char *lua_string);
}
