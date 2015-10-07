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

    namespace {

        template<typename S, typename D>
        struct copy_const_qual {
            using type = D;
        };

        template<typename S, typename D>
        struct copy_const_qual<const S, D> {
            using type = const D;
        };

    }

    template<typename E>
    inline auto to_integral(E e) -> typename std::underlying_type<E>::type {
        return static_cast<typename std::underlying_type<E>::type>(e);
    }

    template<
        typename Enum,
        typename Number = typename copy_const_qual < Enum, typename std::underlying_type<Enum>::type >::type
    >
    inline auto to_integral_ref(Enum & e) -> Number & {
        return reinterpret_cast< Number &>(e);
    }

    template<typename Enum, typename Integer>
    inline auto to_enum(Integer && value) -> Enum {
        static_assert(sizeof Enum >= sizeof Integer, "Enum should have enough room");
        return static_cast<Enum>(value);
    }


}
