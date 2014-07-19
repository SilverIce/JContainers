#pragma once

#include <functional>

namespace collections
{
    class Item;
    class object_base;

    namespace path_resolving {

        void resolvePath(Item& item, const char *cpath, std::function<void(Item *)> itemFunction, bool createMissingKeys = false);

        void resolvePath(object_base *collection, const char *cpath, std::function<void(Item *)> itemFunction, bool createMissingKeys = false);

    }
}
