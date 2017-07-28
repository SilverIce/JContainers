#pragma once

#include <array>
#include <boost/optional.hpp>

#include "collections/collections.h"

namespace collections {

    class array_functions {
    public:

        typedef int32_t index;
        typedef boost::optional<index> maybe_index;

        static maybe_index convertReadIndex(int32_t count, index pyIndex) {
            index index = (pyIndex >= 0 ? pyIndex : (count + pyIndex));

            return maybe_index(count > 0 && index < count,
                index);
        }

        static maybe_index convertReadIndex(const object_base *ar, index pyIndex) {
            return convertReadIndex(ar->u_count(), pyIndex);
        }

        static maybe_index convertWriteIndex(const object_base *ar, index pyIndex) {
            auto count = ar->u_count();
            index index = (pyIndex >= 0 ? pyIndex : (count + pyIndex + 1));

            return maybe_index(index <= count,
                index);
        }

        template<class Index, size_t N>
        static boost::optional<std::array<uint32_t, N> > convertReadIndex(array *ar, const Index(&pyIndexes)[N]) {
            auto count = ar->u_count();

            if (count == 0) {
                return boost::none;
            }

            std::array<uint32_t, N> indexes = { 0 };
            for (size_t i = 0; i < N; ++i) {
                indexes[i] = (pyIndexes[i] >= 0 ? pyIndexes[i] : (count + pyIndexes[i]));

                if (indexes[i] >= count) {
                    return boost::none;
                }
            }

            return indexes;
        }

        template<class Op>
        static void doReadOp(array * obj, index pyIndex, Op& operation) {
            if (!obj) {
                return;
            }

            object_lock g(obj);
            auto idx = convertReadIndex(obj, pyIndex);
            if (idx) {
                operation(*idx);
            }
        }

        template<class Op, class Index, size_t N>
        static void doReadOp(array * obj, const Index(&pyIndex)[N], Op& operation) {
            if (!obj) {
                return;
            }

            object_lock g(obj);
            auto idx = convertReadIndex(obj, pyIndex);
            if (idx) {
                operation(*idx);
            }
        }

        template<class Op>
        static void doWriteOp(array * obj, index pyIndex, Op& operation) {
            if (!obj) {
                return;
            }

            object_lock g(obj);
            auto idx = convertWriteIndex(obj, pyIndex);
            if (idx) {
                operation(*idx);
            }
        }
    };

    //template<class T>
    struct map_key_checker {
        static bool check(const std::string& s)  { return !s.empty(); }
        static bool check(const char *s)  { return s != nullptr && *s; }
        static bool check(TESForm *f)  { return f != nullptr; }
        static bool check(FormId f)  { return f != FormId::Zero; }
        static bool check(const form_ref& f)  { return f.is_not_expired(); }
        static bool check(const form_ref_lightweight& f)  { return f.is_not_expired(); }
        static bool check(int32_t)  { return true; }
    };

    template<class T>
    class map_functions_templ {
    public:
        using key_checker = map_key_checker/*<T>*/;
        ///typedef typename T::key_type key_type;

        template<class Op, class R,/* class RAlter, */class key_type>
        static R doReadOpR(T * obj, const key_type& key, R default, Op& operation) {
            if (obj && key_checker::check(key)) {
                object_lock g(obj);
                item *itm = obj->u_get(key);
                return itm ? operation(*itm) : default;
            }
            else {
                return default;
            }
        }

        template<class Op, class key_type>
        static void doReadOp(T * obj, const key_type& key, Op& operation) {
            if (obj && key_checker::check(key)) {
                object_lock g(obj);
                item *itm = obj->u_get(key);
                if (itm) {
                    operation(*itm);
                }
            }
        }

        // force write into an item
        template<class Op, class key_type>
        static void doWriteOp(T * obj, const key_type& key, Op& operation) {
            if (obj && key_checker::check(key)) {
                object_lock g(obj);
                item &itm = obj->u_get_or_create(key);
                operation(itm);
            }
        }

        template<class KeyFunc, class KeyTypeIn>
        static void nextKey(const T *obj, const KeyTypeIn& lastKey, KeyFunc keyFunc) {
            if (obj) {
                object_lock g(obj);
                auto& container = obj->u_container();
                if (key_checker::check(lastKey)) {
                    auto itr = container.find(lastKey);
                    auto end = container.end();
                    if (itr != end && (++itr) != end) {
                        keyFunc(itr->first);
                    }
                }
                else if (container.empty() == false) {
                    keyFunc(container.begin()->first);
                }
            }
        }

        struct equal_to {
            template<class T, class D>
            inline bool operator()(T& t, D& d) const {
                return t == d;
            }
        };

        template<class KeyTypeIn, class KeyComparer = equal_to>
        static KeyTypeIn nextKey_forPapyrus(const T *obj, const KeyTypeIn& lastKey,
            const KeyTypeIn& endKey, const KeyComparer key_equality = equal_to{})
        {
            if (obj) {
                object_lock g(obj);
                auto& container = obj->u_container();

                if (container.empty()) {
                    return endKey;
                }
                else if (key_equality(lastKey, endKey) == false) { // keys aren't equal
                    auto itr = obj->u_find_iterator(lastKey);
                    const auto end = container.cend();

                    if (itr == end) {
                        return endKey;
                    }

                    while (++itr != end) {
                        if (key_equality(itr->first, endKey) == false) {
                            return itr->first;
                        }
                    }
                }
                // start iteration
                else {
                    auto itr = container.begin();
                    const auto end = container.cend();

                    while (itr != end) {
                        if (key_equality(itr->first, endKey) == false) {
                            return itr->first;
                        }
                        ++itr;
                    }
                }
            }

            return endKey;
        }

        template<class KeyFunc>
        static void getNthKey(const T *obj, int32_t keyIdx, KeyFunc keyFunc) {
            if (obj) {
                object_lock g(obj);
                auto idx = array_functions::convertReadIndex(obj, keyIdx);
                if (idx) {
                    int32_t count = obj->u_count();
                    if (*idx < count / 2) {
                        auto itr = obj->u_container().begin();
                        for (int32_t i = 0, dest = *idx; i != dest; ++i, ++itr) {}
                        keyFunc(itr->first);
                    }
                    else {
                        auto itr = obj->u_container().rbegin();
                        for (int32_t i = (count - 1), dest = *idx; i != dest; --i, ++itr) {}
                        keyFunc(itr->first);
                    }
                }
            }
        }
    };


    typedef map_functions_templ<map> map_functions;
    typedef map_functions_templ<form_map> formmap_functions;
    typedef map_functions_templ<integer_map> integer_map_functions;

}
