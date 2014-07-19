#include "path_resolving.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/find_end.hpp>

#include <functional>

#include "collections.h"
#include "tes_context.h"

#include "collection_operators.h"

namespace collections
{
    namespace path_resolving {

        namespace bs = boost;
        namespace ss = std;

        typedef ss::function<Item* (object_base*)> NodeFunc;
        typedef ss::function<void (const Item& , Item&)> ElementFunc;
        typedef boost::iterator_range<const char*> path_type;

        struct state {
            bool succeed;
            path_type path;
            NodeFunc nodeGetter;
            object_base *object;

            state(bool _succeed, const NodeFunc& _node, object_base *_object, const path_type& _path) {
                succeed = _succeed;
                nodeGetter = _node;
                path = _path;
                object = _object;
            }

            state(bool _succeed, const state& st) {
                succeed = _succeed;
                nodeGetter = st.nodeGetter;
                path = st.path;
                object = st.object;
            }
        };

        template<class T, class V>
        static bool _map_visit_helper(T *container, path_type path, const V& func)
        {
            if (!container || path.empty()) {
                return false;
            }

            bool isKeyVisit = false;

            path_type::iterator rightPath;

            if (bs::istarts_with(path, ".key")) {
                isKeyVisit = true;
                rightPath = path.begin() + bs::size(".key") - 1;
            }
            else if (bs::istarts_with(path, ".value")) {
                isKeyVisit = false;
                rightPath = path.begin() + bs::size(".value") - 1;
            } else {
                return false;
            }

            auto copy = container->container_copy();

            if (isKeyVisit) {
                for (auto &pair : copy) {
                    Item itm(pair.first);
                    resolvePath(itm, rightPath, func);
                }
            } else { // is value visit
                for (auto &pair : copy) {
                    resolvePath(pair.second, rightPath, func);
                }
            }

            return true;
        }

        void resolvePath(Item& item, const char *cpath, std::function<void(Item *)>  itemFunction, bool createMissingKeys) {
            if (!cpath) {
                return;
            }

            if (item.object()) {
                resolvePath(item.object(), cpath, itemFunction, createMissingKeys);
            }
            else if (!*cpath) {
                itemFunction(&item);
            }
        }

        void resolvePath(object_base *collection, const char *cpath, std::function<void (Item *)>  itemFunction, bool createMissingKeys) {

            if (!collection || !cpath) {
                return;
            }

            // path is empty -> just visit collection
            if (!*cpath) {
                Item itm(collection);
                itemFunction(&itm);
                return ;
            }

            namespace bs = boost;
            namespace ss = std;

            auto path = bs::make_iterator_range(cpath, cpath + strnlen_s(cpath, 1024));

            auto operatorRule = [createMissingKeys](const state &st) -> state {
                const auto& path = st.path;

                if (!bs::starts_with(path, "@") || path.size() < 2) {
                    return state(false, st);
                }

                auto begin = path.begin() + 1;
                auto end = bs::find_if(path_type(begin, path.end()), bs::is_any_of("."));
                auto operationStr = ss::string(begin, end);

                if (operationStr.empty()) {
                    return state(false, st);
                }

                auto rightPath = path_type(end, path.end());

                auto collection = st.nodeGetter(st.object)->object();

                if (!collection) {
                    return state(false, st);
                }

                auto opr = collection_operators::get_operator(operationStr);

                if (!opr) {
                    return state(false, st);
                }

                Item sharedItem;

                auto itemVisitFunc = [&](Item *item) {
                    if (item) {
                        opr->func(*item, sharedItem);
                    }
                };

                if (collection->as<array>()) {

                    // have to copy array to prevent its modification during iteration
                    auto array_copy = collection->as<array>()->container_copy();

                    for (auto &itm : array_copy) {
                        resolvePath(itm, rightPath.begin(), itemVisitFunc);
                    }
                }
                else if (collection->as<map>()) {
                    _map_visit_helper(collection->as<map>(), rightPath, itemVisitFunc);
                }
                else if (collection->as<form_map>()) {
                    _map_visit_helper(collection->as<form_map>(), rightPath, itemVisitFunc);
                }

                return state(true,
                    [=](object_base *) mutable -> Item* { return &sharedItem;},
                    nullptr,
                    path_type());
            };

            auto mapRule = [createMissingKeys](const state &st) -> state {

                const auto& path = st.path;

                if (!bs::starts_with(path, ".") || path.size() < 2) {
                    return state(false, st);
                }

                auto begin = path.begin() + 1;
                auto end = bs::find_if(path_type(begin, path.end()), bs::is_any_of(".["));

                if (begin == end) {
                    return state(false, st);
                }

                object_lock lock(st.object);
                auto node = st.nodeGetter(st.object);

                if (createMissingKeys && node && node->isNull()) {
                    *node = map::object(tes_context::instance());
                }

                auto container = node ? node->object() : nullptr;

                if (!container) {
                    return state(false, st);
                }

                return state(   true,
                                    [=](object_base *container) {
                                        Item *itemPtr = nullptr;

                                        if (auto obj = container->as<map>()) {
                                            ss::string key(begin, end);
                                            itemPtr = obj->u_find(key);

                                            if (!itemPtr && createMissingKeys) {
                                                obj->u_setValueForKey(key, Item());
                                                itemPtr = obj->u_find(key);
                                            }
                                        }

                                        return itemPtr;
                                },
                                container,
                                path_type(end, path.end()) );
            };

            auto arrayRule = [](const state &st) -> state {

                const auto& path = st.path;

                if (!bs::starts_with(path, "[") || path.size() < 3) {
                    return state(false, st);
                }

                auto begin = path.begin() + 1;
                auto end = bs::find_if(path_type(begin, path.end()), bs::is_any_of("]"));
                auto indexRange = path_type(begin, end);

                if (indexRange.empty()) {
                    return state(false, st);
                }

                UInt32 indexOrFormId = 0;
                try {
                    indexOrFormId = std::stoul(ss::string(indexRange.begin(), indexRange.end()), nullptr, 0);
                }
                catch (const std::invalid_argument& ) {
                    return state(false, st);
                }
                catch (const std::out_of_range& ) {
                    return state(false, st);
                }

                object_lock lock(st.object);
                auto container = st.nodeGetter(st.object)->object();

                if (!container) {
                    return state(false, st);
                }

                return state(   true,
                                [=](object_base* container) {
                                    if (container->as<array>()) {
                                        return container->as<array>()->u_getItem(indexOrFormId);
                                    }
                                    else if (container->as<form_map>()) {
                                        return container->as<form_map>()->u_find((FormId)indexOrFormId);
                                    } else {
                                        return (Item *)nullptr;
                                    } 
                                },
                                container,
                                path_type(end + 1, path.end()) );
            };

            const ss::function<state (const state &st) > rules[] = {operatorRule, mapRule, arrayRule};

            Item root(collection);
            state st(true, [&](object_base*) { return &root;}, collection, path);

            while (true) {

                bool anySucceed = false;
                for (auto& func : rules) {
                    st = func(st);
                    anySucceed = st.succeed;
                    if (anySucceed) {
                        break;
                    }
                }

                if (st.path.empty() && anySucceed) {

                    if (st.object) {
                        object_lock lock(st.object);
                        itemFunction( st.nodeGetter(st.object));
                    } else {
                        itemFunction( st.nodeGetter(nullptr));
                    }

                    break;
                }

                if (!anySucceed) {
                    itemFunction(nullptr);
                    break;
                }
            }
        }

    }
}
