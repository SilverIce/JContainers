#include "path_resolving.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/find_end.hpp>

#include <functional>

#include "collections.h"

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

            auto copy = container->container();

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

        void resolvePath(Item& item, const char *cpath, std::function<void (Item *)>  itemFunction) {
            if (!cpath) {
                return;
            }

            if (item.object()) {
                resolvePath(item.object(), cpath, itemFunction);
            }
            else if (!*cpath) {
                itemFunction(&item);
            }
        }

        void resolvePath(object_base *collection, const char *cpath, std::function<void (Item *)>  itemFunction) {

            if (!collection || !cpath) {
                return;
            }

            if (collection && !*cpath) {
                Item itm(collection);
                itemFunction(&itm);
                return ;
            }

            namespace bs = boost;
            namespace ss = std;

            auto path = bs::make_iterator_range(cpath, cpath + strnlen_s(cpath, 1024));

            auto operatorRule = [](const state &st) -> state {
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

                    // have to copy array to prevent it modification during iteration
                    auto array_copy = collection->as<array>()->_array;

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

            auto mapRule = [](const state &st) -> state {

                const auto& path = st.path;

                if (!bs::starts_with(path, ".") || path.size() < 2) {
                    return state(false, st);
                }

                auto begin = path.begin() + 1;
                auto end = bs::find_if(path_type(begin, path.end()), bs::is_any_of(".["));

                object_lock lock(st.object);
                auto node = st.nodeGetter(st.object);
                auto container = node ? node->object() : nullptr;

                if (begin == end || !container) {
                    return state(false, st);
                }

                return state(   true,
                                    [=](object_base *container) {
                                        return container->as<map>() ? container->as<map>()->u_find(ss::string(begin, end)) : nullptr;
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

                int indexOrFormId = 0;
                try {
                    indexOrFormId = std::stoi(ss::string(indexRange.begin(), indexRange.end()), nullptr, 0);
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
