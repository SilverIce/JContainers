#pragma once

#include <array>
#include <boost/optional.hpp>

#include "collections.h"

namespace collections {

    class array_functions {
    public:

        typedef int32_t index;
        typedef boost::optional<index> maybe_index;

        static maybe_index convertReadIndex(const object_base *ar, index pyIndex) {
            auto count = ar->u_count();
            index index = (pyIndex >= 0 ? pyIndex : (count + pyIndex));

            return maybe_index(count > 0 && index < count,
                index);
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
                return false;
            }

            std::array<uint32_t, N> indexes;
            for (size_t i = 0; i < N; ++i) {
                indexes[i] = (pyIndexes[i] >= 0 ? pyIndexes[i] : (count + pyIndexes[i]));

                if (indexes[i] >= count) {
                    return false;
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
        static bool check(FormId f)  { return f != FormZero; }
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
                Item *itm = obj->u_find(key);
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
                Item *itm = obj->u_find(key);
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
                Item &itm = obj->u_container()[key];
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
