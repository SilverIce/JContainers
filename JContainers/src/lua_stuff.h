#pragma once

namespace collections {

    class Item;
    class object_base;

    Item process_apply_func(object_base *object, const char *lua_string);
}
