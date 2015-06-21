#pragma once

#include <functional>
#include <type_traits>
#include <boost/optional.hpp>
#include <boost/variant/variant.hpp>

#include "collections/collections.h"

namespace collections
{
    class item;
    class object_base;
    class tes_context;

    namespace path_resolving {

        void resolve(tes_context& ctx, item& target, const char *cpath, std::function<void(item *)> itemFunction, bool createMissingKeys = false);

        void resolve(tes_context& ctx, object_base *target, const char *cpath, std::function<void(item *)> itemFunction, bool createMissingKeys = false);

        template<class T>
        inline T _resolve(tes_context& ctx, object_base *target, const char *cpath, T def = T(0)) {
            resolve(ctx, target, cpath, [&](item *itm) {
                if (itm) {
                    def = itm->readAs<T>();
                }
            });

            return def;
        }
    }

    // ca - collection access
    namespace ca {
        namespace bs = boost;

        using key_variant = boost::variant<int32_t, std::string, FormId>;

        struct u_access_value_helper {
            template<class Collection>
            item* operator () (Collection& collection, const key_variant& key) {
                if (auto idx = bs::get<typename Collection::key_type>(&key)) {
                    return collection.u_get(*idx);
                }
                return nullptr;
            }
        };

        inline auto u_access_value(object_base& collection, const key_variant& key) -> item* {
            return perform_on_object_and_return<item* >(collection, u_access_value_helper(), key);
        };
        // 

        template<class Value>
        struct u_assign_value_helper {
            template<class T>
            item* operator()(T& obj, const key_variant& key, Value&& value) {
                if (auto idx = bs::get<typename T::key_type>(&key)) {
                    return obj.u_set(*idx, std::forward<Value>(value));
                }
                return nullptr;
            }
        };

        template<class Value>
        inline auto u_assign_value(object_base& collection, const key_variant& key, Value&& value) -> item* {
            return perform_on_object_and_return<item* >(collection, u_assign_value_helper<Value>(), key, std::forward<Value>(value));
        }

        struct accesss_info {
            object_base& collection;
            key_variant key;
        };

        template<class T>
        inline bs::optional<T> _opt_from_pointer(T* t) {
            return t ? bs::optional<T>(*t) : bs::none;
        }

        enum access_way {
            constant,
            creative,
        };

        bs::optional<accesss_info> access_constant(object_base& tree, const char* path);
        bs::optional<accesss_info> access_creative(object_base& tree, const char* path);


        inline bs::optional<item> get(object_base& target, const char *cpath) {
            auto ac_info = access_constant(target, cpath);
            if (ac_info) {
                object_lock g(ac_info->collection);
                auto itmPtr = u_access_value(ac_info->collection, ac_info->key);
                return _opt_from_pointer(itmPtr);
            }
            else {
                return bs::none;
            }
        }

        template<class Func, class ...Args>
        inline bool visit_value(object_base& target, const char *cpath, access_way way, Func f, Args&&... args) {
            auto ac_info = (way == constant ? access_constant(target, cpath) : access_creative(target, cpath));
            if (ac_info) {
                object_lock g(ac_info->collection);
                auto itmPtr = u_access_value(ac_info->collection, ac_info->key);
                if (itmPtr) {
                    f(*itmPtr, std::forward<Args>(args)...);
                }
                return itmPtr != nullptr;
            }
            else {
                return false;
            }
        }

        template<class Value>
        inline bs::optional<Value> get(object_base& target, const char *cpath) {
            auto ac_info = access_constant(target, cpath);
            if (ac_info) {
                object_lock g(ac_info->collection);
                auto itmPtr = u_access_value(ac_info->collection, ac_info->key);
                return itmPtr ? _opt_from_pointer(itmPtr->get<Value>()) : bs::none;
            }
            else {
                return bs::none;
            }
        }

        template<class Value>
        bool assign(object_base& target, const char *cpath, Value&& value, access_way way = constant) {
            auto ac_info = (way == constant ? access_constant(target, cpath) : access_creative(target, cpath));
            if (ac_info) {
                object_lock g(ac_info->collection);
                if (way == constant) {
                    auto itmPtr = u_access_value(ac_info->collection, ac_info->key);
                    if (itmPtr) {
                        *itmPtr = std::forward<Value>(value);
                    }
                    return itmPtr != nullptr;
                } else {
                    return u_assign_value(ac_info->collection, ac_info->key, std::forward<Value>(value)) != nullptr;
                }
            }
            else {
                return false;
            }
        }

        template<class Value>
        bool assign_creative(object_base& target, const char *cpath, Value&& value) {
            return assign(target, cpath, std::forward<Value>(value), creative);
        }

    }
}
