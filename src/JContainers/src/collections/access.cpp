#include "collections/access.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/find_end.hpp>

#include <functional>

#include "forms/form_handling.h"
#include "collections/collections.h"
#include "collections/context.h"
#include "collections/operators.h"
#include "util/cstring.h"

namespace collections
{
    namespace path_resolving {

        namespace bs = boost;
        namespace ss = std;

        typedef ss::function<item* (object_base*)> NodeFunc;
        typedef ss::function<void (const item& , item&)> ElementFunc;
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

        template<class T>
        static bool _map_visit_helper(tes_context& context, T& container, path_type path, std::function<void(item *)>&& function)
        {
            if (path.empty()) {
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

            auto copy = container.container_copy();

            if (isKeyVisit) {
                item itm;
                for (auto &pair : copy) {
                    itm = pair.first;
                    resolve(context, itm, rightPath, function);
                }
            } else { // is value visit
                for (auto &pair : copy) {
                    resolve(context, pair.second, rightPath, function);
                }
            }

            return true;
        }

        void resolve(tes_context& context, item& target, const char *cpath,
            const std::function<void(item *)>& itemFunction, bool createMissingKeys)
        {
            if (!cpath) {
                return;
            }

            if (target.object()) {
                resolve(context, target.object(), cpath, itemFunction, createMissingKeys);
            }
            else if (!*cpath) {
                itemFunction(&target);
            }
        }

        void resolve(tes_context& context, object_base *collection, const char *cpath,
            const std::function<void(item *)>& itemFunction, bool createMissingKeys)
        {

            if (!collection || !cpath) {
                return;
            }

            // path is empty -> just visit collection
            if (!*cpath) {
                item itm(collection);
                itemFunction(&itm);
                return ;
            }

            namespace bs = boost;
            namespace ss = std;

            auto path = bs::make_iterator_range(cpath, cpath + strnlen_s(cpath, 1024));

            auto operatorRule = [createMissingKeys, &context](const state &st) -> state {
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

                auto opr = operators::get_operator(operationStr.c_str());

                if (!opr) {
                    return state(false, st);
                }

                item sharedItem;

                auto itemVisitFunc = [&](item *item) {
                    if (item) {
                        opr->func(*item, sharedItem);
                    }
                };

                struct 
                {
                    decltype(context)       context;
                    decltype(rightPath)     *rightPath;
                    decltype(itemVisitFunc) *visitFunc;

                    void operator()(array& arr) {
                        // have to copy array to prevent its modification during iteration
                        auto array_copy = arr.container_copy();
                        for (auto &itm : array_copy) {
                            resolve(context, itm, rightPath->begin(), *visitFunc);
                        }
                    }
                    void operator()(map& cnt) {
                        _map_visit_helper(context, cnt, *rightPath, *visitFunc);
                    }
                    void operator()(form_map& cnt) {
                        _map_visit_helper(context, cnt, *rightPath, *visitFunc);
                    }
                    void operator()(integer_map& cnt) {
                        _map_visit_helper(context, cnt, *rightPath, *visitFunc);
                    }

                } helper{ context, &rightPath, &itemVisitFunc };

                perform_on_object(*collection, helper);

                return state(true,
                    [=](object_base *) mutable -> item* { return &sharedItem;},
                    nullptr,
                    path_type());
            };

            auto mapRule = [createMissingKeys, &context](const state &st) -> state {

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
                    *node = map::object(context);
                }

                auto container = node ? node->object() : nullptr;

                if (!container) {
                    return state(false, st);
                }

                return state(   true,
                                    [=](object_base *container) {
                                        item *itemPtr = nullptr;

                                        if (auto obj = container->as<map>()) {
                                            ss::string key(begin, end);
                                            itemPtr = obj->u_get(key);

                                            if (!itemPtr && createMissingKeys) {
                                                obj->u_set(key, item());
                                                itemPtr = obj->u_get(key);
                                            }
                                        }

                                        return itemPtr;
                                },
                                container,
                                path_type(end, path.end()) );
            };

            auto arrayRule = [&context](const state &st) -> state {

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

                int32_t indexOrFormId = 0;
                FormId frmId = FormId::Zero;

                if (!forms::is_form_string(indexRange.begin())) {
                    try {
                        indexOrFormId = std::stoi(ss::string(indexRange.begin(), indexRange.end()), nullptr, 0);
                    }
                    catch (const std::invalid_argument& ) {
                        return state(false, st);
                    }
                    catch (const std::out_of_range& ) {
                        return state(false, st);
                    }
                }
                else {
                    if (auto fId = forms::string_to_form (indexRange.begin ()))
                        frmId = *fId;
                    else
                        return state (false, st);
                }

                object_lock lock(st.object);
                auto container = st.nodeGetter(st.object)->object();

                if (!container) {
                    return state(false, st);
                }

                return state(   true,
                                [=, &context](object_base* container) {
                                    if (container->as<array>()) {
                                        return container->as<array>()->u_get(indexOrFormId);
                                    }
                                    else if (container->as<form_map>()) {
                                        return container->as<form_map>()->u_get(make_weak_form_id(frmId, context));
                                    }
                                    else if (container->as<integer_map>()) {
                                        return container->as<integer_map>()->u_get(indexOrFormId);
                                    }
                                    else {
                                        return (item *)nullptr;
                                    } 
                                },
                                container,
                                path_type(end + 1, path.end()) );
            };

            const ss::function<state (const state &st) > rules[] = {operatorRule, mapRule, arrayRule};

            item root(collection);
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

    namespace ca {

        enum {
            string_path_length_max = 1024,
            string_number_length_max = 20,
        };

        namespace bs = boost;

        using std::forward;
        using std::string;

        namespace {
        using cstring = util::cstring;
        //using keys = std::vector<key_variant>;

        template<class R, class Arg, class F1, class ... F>
        boost::optional<R> parse_path_helper(Arg&& a, F1&& f1, F&& ... funcs) {
            auto result = f1(std::forward<Arg>(a));
            if (result) {
                return result;
            }
            else {
                return parse_path_helper<R>(std::forward<Arg>(a), std::forward<F>(funcs)...);
            }
        };

        template<class R, class Arg>
        boost::none_t parse_path_helper(Arg&&) {
            return boost::none;
        }

        struct key_and_rest {
            key_variant key;
            cstring rest_of_path;
        };

        template<class Context>
        bs::optional<key_and_rest> parse_path(Context& context, const cstring& path) {

            if (path.empty()) {
                return bs::none;
            }

            auto mapRule = [](const cstring &path)-> bs::optional < key_and_rest > {

                if (!bs::starts_with(path, ".") || path.size() < 2) {
                    return bs::none;
                }

                auto begin = path.begin() + 1;
                auto end = bs::find_if(cstring(begin, path.end()), bs::is_any_of(".["));

                if (begin == end) {
                    return bs::none;
                }

                string str(begin, end);
                return key_and_rest{ std::move(str), cstring(end, path.end()) };
            };

            auto arrayRule = [&context](const cstring &path)-> bs::optional < key_and_rest > {

                if (!bs::starts_with(path, "[") || path.size() < 3) {
                    return bs::none;
                }

                auto begin = path.begin() + 1;
                auto end = bs::find_if(cstring(begin, path.end()), bs::is_any_of("]"));
                auto indexRange = cstring(begin, end);

                if (indexRange.empty()) {
                    return bs::none;
                }

                if (!forms::is_form_string(indexRange.begin())) {
                    int32_t index = 0;
                    try {
                        index = std::stoi(string(indexRange.begin(), indexRange.end()), nullptr, 0);
                    }
                    catch (const std::invalid_argument&) {
                        return bs::none;
                    }
                    catch (const std::out_of_range&) {
                        return bs::none;
                    }

                    return key_and_rest{ index, cstring(end + 1, path.end()) };
                }
                else {
                    auto fId = forms::string_to_form (indexRange.begin ());
                    if (!fId)
                        return bs::none;

                    return key_and_rest{ make_weak_form_id(*fId, context), cstring(end + 1, path.end()) };
                }
            };

            return parse_path_helper<key_and_rest>(path, arrayRule, mapRule);
        }

        struct constant_accessor {
            static bs::optional<object_base*> access_value(object_base& collection, const bs::optional<key_and_rest>& key) {
                if (!key) {
                    return bs::none;
                }
                object_lock lock(collection);
                auto itemPtr = u_access_value(collection, key->key);
                return itemPtr ? bs::make_optional(itemPtr->object()) : bs::none;
            }
        };

        struct creative_accessor {
            static bs::optional<object_base*> access_value(object_base& collection, const bs::optional<key_and_rest>& key) {
                if (!key) {
                    return bs::none;
                }
                object_lock lock(collection);
                auto itemPtr = u_access_value(collection, key->key);
                /*  is int-map and key is int
                is form-map
                is map and key is string
                failure
                */
                if (!itemPtr) {
                    itemPtr = u_assign_value(collection, key->key, item());
                    auto next_key = parse_path(HACK_get_tcontext(collection), key->rest_of_path);
                    if (itemPtr && next_key) {
                        struct creator : public bs::static_visitor<object_base*> {
                            object_context* ctx;
                            explicit creator(object_context* c) : ctx(c) {}

                            object_base* operator ()(const int32_t& k) const { return &integer_map::object(*ctx); }
                            object_base* operator ()(const string& k) const { return &map::object(*ctx); }
                            object_base* operator ()(const form_ref& k) const { return &form_map::object(*ctx); }
                        };
                        *itemPtr = bs::apply_visitor(creator(&collection.context()), next_key->key);
                    }
                }

                return itemPtr ? bs::make_optional(itemPtr->object()) : bs::none;
            }

        };

        template<class access_value>
        struct last_kv_pair_retriever {

            /* Propotype:

            retrieve obj -> str -> (obj, key)
            retrieve obj str = recurs obj[(key, rest)] (key, rest) obj
                                where
                                    (key, rest) = parse str

            recurs v    (key, rest)     src = recurs v[(nkey, nrest)] (nkey, nrest) v where (nkey, nrest) = parse rest
            recurs V    (key, Nothing)  src = (src, key)
            recurs None (key, Nothing)  src = (src, key) ????
            recurs _    _               _   = Nothing
            
            */

            static bs::optional<accesss_info> retrieve(object_base& collection, const cstring& path) {
                auto key_opt = parse_path(HACK_get_tcontext(collection), path);
                return recurs(access_value::access_value(collection, key_opt), key_opt, collection);
            }

            static bs::optional<accesss_info> recurs(bs::optional<object_base*>&& value, const bs::optional<key_and_rest>& k, object_base& source) {
                if (!k) {
                    return bs::none;
                }

                object_base* as_object = value.get_value_or(nullptr);

                if (k->rest_of_path.empty()) {
                    return accesss_info{ source, std::move(k->key) };
                }
                else if (as_object && !k->rest_of_path.empty()) {
                    auto next_key = parse_path(HACK_get_tcontext(source), k->rest_of_path);
                    return recurs(access_value::access_value(*as_object, next_key), next_key, *as_object);
                }
                else {
                    return bs::none;
                }
            }
        };
        }

        bs::optional<accesss_info> access_constant(object_base& collection, const char* cpath) {
            auto all_path = util::make_cstring_safe(cpath, string_path_length_max);
            return last_kv_pair_retriever<constant_accessor>::retrieve(collection, all_path);
        }

        bs::optional<accesss_info> access_creative(object_base& collection, const char* cpath) {
            auto all_path = util::make_cstring_safe(cpath, string_path_length_max);
            return last_kv_pair_retriever<creative_accessor>::retrieve(collection, all_path);
        }
    }
}
