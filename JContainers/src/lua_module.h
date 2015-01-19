#pragma once

namespace collections {
    class object_base;
    class Item;
}

namespace boost {
    template<class T> class optional;
}

namespace lua {
    boost::optional<collections::Item> eval_lua_function(collections::object_base *object, const char *lua_string);
}
