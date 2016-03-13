#pragma once

#include <type_traits>

namespace util {

    namespace {
        struct tree_erase_if_eraser {
            template<class TreeContainer>
            typename TreeContainer::iterator operator()(TreeContainer& cnt, typename TreeContainer::const_iterator itr) const {
                return cnt.erase(itr);
            }
        };
    }

    template<class TreeContainer, class Predicate, class Eraser = tree_erase_if_eraser>
    void tree_erase_if(TreeContainer&& container, Predicate&& pred, Eraser&& eraser = Eraser{}) {
        for (auto itr = container.begin(); itr != container.end();) {
            if (pred(*itr)) {
                itr = eraser(container, itr);
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
        return reinterpret_cast<Number &>(e);
    }

    template<typename Enum, typename Integer>
    inline auto to_enum(Integer && value) -> Enum {
        static_assert(sizeof Enum >= sizeof Integer, "Enum should have enough room");
        return static_cast<Enum>(value);
    }

    // Helps choose between const and non-const iterator
    template<class ContainerType>
    using choose_iterator = typename std::conditional< std::is_const<ContainerType>::value,
        typename ContainerType::const_iterator, typename ContainerType::iterator>::type;
}
