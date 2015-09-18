#pragma once

#include <type_traits>

namespace util {

    template<class TreeContainer, class Predicate>
    void tree_erase_if(TreeContainer&& container, Predicate&& pred) {
        for (auto itr = container.begin(); itr != container.end();) {
            if (pred(*itr)) {
                itr = container.erase(itr);
            }
            else {
                ++itr;
            }
        }
    }

    template<typename E>
    inline auto to_integral(E e) -> typename std::underlying_type<E>::type {
        return static_cast<typename std::underlying_type<E>::type>(e);
    }

}
