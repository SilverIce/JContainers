#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/find_end.hpp>

#include <functional>

#include "skse/GameData.h"

#include "collection_operators.h"

namespace collections {

    inline std::unique_ptr<FILE, decltype(&fclose)> make_unique_file(FILE *file) {
        return std::unique_ptr<FILE, decltype(&fclose)> (file, &fclose);
    }

    static const char * kJSerializedFormData = "__formData";
    static const char * kJSerializedFormDataSeparator = "|";

    class json_handling {
    public:

        static void resolvePath(Item& item, const char *cpath, std::function<void (Item *)>  itemFunction) {
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

        typedef std::function<Item* (object_base*)> NodeFunc;
        typedef std::function<void (const Item& , Item&)> ElementFunc;
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
            namespace bs = boost;
            namespace ss = std;

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

        static void resolvePath(object_base *collection, const char *cpath, std::function<void (Item *)>  itemFunction) {

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

        static object_base * readJSONFile(const char *path) {
            auto cj = cJSONFromFile(path);
            auto res = readCJSON(cj);
            cJSON_Delete(cj);
            return res;
        }

        static object_base * readJSONData(const char *text) {
            auto cj = cJSON_Parse(text);
            auto res = readCJSON(cj);
            cJSON_Delete(cj);
            return res;
        }

        static object_base * readCJSON(cJSON *value) {
            if (!value) {
                return nullptr;
            }

            object_base *obj = nullptr;
            if (value->type == cJSON_Array || value->type == cJSON_Object) {
                Item itm = makeItem(value);
                obj = itm.object();
            }

            return obj;
        }

        static cJSON * cJSONFromFile(const char *path) {
            using namespace std;

            auto file = make_unique_file(fopen(path, "r"));
            if (!file.get())
                return 0;

            char buffer[1024];
            std::vector<char> bytes;
            while (!ferror(file.get()) && !feof(file.get())) {
                size_t readen = fread(buffer, 1, sizeof(buffer), file.get());
                if (readen > 0) {
                    bytes.insert(bytes.end(), buffer, buffer + readen);
                }
                else  {
                    break;
                }
            }
            bytes.push_back(0);

            return cJSON_Parse(&bytes[0]);
        }

        static  array * makeArray(cJSON *val) {
            array *ar = array::object();

            int count = cJSON_GetArraySize(val);
            for (int i = 0; i < count; ++i) {
            	ar->u_push(makeItem(cJSON_GetArrayItem(val, i)));
            }

            return ar;
        }

        static object_base * makeObject(cJSON *val) {

            object_base *object = nullptr;
            bool isFormMap = cJSON_GetObjectItem(val, kJSerializedFormData) != nullptr;

            if (isFormMap) {
                auto obj = form_map::object();
                object = obj;

                int count = cJSON_GetArraySize(val);
                for (int i = 0; i < count; ++i) {
                    auto itm = cJSON_GetArrayItem(val, i);
                    FormId key = formIdFromString(itm->string);

                    if (key) {
                        obj->u_setValueForKey(key, makeItem(itm));
                    }
                }
            } else {

                auto obj = map::object();
                object = obj;

                int count = cJSON_GetArraySize(val);
                for (int i = 0; i < count; ++i) {
                    auto itm = cJSON_GetArrayItem(val, i);
                    obj->u_setValueForKey(itm->string, makeItem(itm));
                }
            }

            return object;
        }

        static Item makeItem(cJSON *val) {
            Item item;
            int type = val->type;
            if (type == cJSON_Array) {
                auto ar = makeArray(val);
                item.setObjectVal(ar);
            } else if (type == cJSON_Object) {
                auto ar = makeObject(val);
                item.setObjectVal(ar);
            } else if (type == cJSON_String) {

                bool isFormData = strncmp(val->valuestring, kJSerializedFormData, strlen(kJSerializedFormData)) == 0;

                if (!isFormData) {
                    item.setStringVal(val->valuestring);
                } else {
                    item.setFormId(formIdFromString(val->valuestring));
                }

            } else if (type == cJSON_Number) {
                if (std::floor(val->valuedouble) == val->valuedouble) {
                    item.setInt(val->valueint);
                } else {
                    item.setFlt(val->valuedouble);
                }
            }

            return item;
        }

        typedef std::set<object_base*> collection_set;

        static cJSON * createCJSON(object_base & collection) {
            collection_set serialized;
            return createCJSONNode(Item(&collection), serialized);
        }

        static char * createJSONData(object_base & collection) {
            collection_set serialized;
            auto node = createCJSONNode(Item(&collection), serialized);
            char *data = cJSON_Print(node);
            cJSON_Delete(node);
            return data;
        }

        static cJSON * createCJSONNode(const Item& item, collection_set& serializedObjects) {

            cJSON *val = nullptr;

            ItemType type = item.type();

            if (type == ItemTypeObject && item.object()) {

                auto obj = item.object();

                if (serializedObjects.find(obj) != serializedObjects.end()) {
                    goto createNullNode;
                    // do not serialize object twice
                }

                serializedObjects.insert(obj);

                object_lock g(obj);

                if (obj->as<array>()) {

                    val = cJSON_CreateArray();
                    array *ar = obj->as<array>();
                    for (auto& itm : ar->_array) {
                        auto cnode = createCJSONNode(itm, serializedObjects);
                        if (cnode) {
                            cJSON_AddItemToArray(val, cnode);
                        }
                    }
                }
                else if (obj->as<map>()) {

                    val = cJSON_CreateObject();
                    for (auto& pair : obj->as<map>()->container()) {
                         auto cnode = createCJSONNode(pair.second, serializedObjects);
                         if (cnode) {
                             cJSON_AddItemToObject(val, pair.first.c_str(), cnode);
                         }
                    }
                }
                else if (obj->as<form_map>()) {

                    val = cJSON_CreateObject();

                    cJSON_AddItemToObject(val, kJSerializedFormData, cJSON_CreateNull());

                    for (auto& pair : obj->as<form_map>()->container()) {
                        auto cnode = createCJSONNode(pair.second, serializedObjects);
                        if (cnode) {
                            auto key = formIdToString(pair.first);
                            if (!key.empty()) {
                                cJSON_AddItemToObject(val, key.c_str(), cnode);
                            }
                        }
                    }
                }
            }
            else if (type == ItemTypeCString) {

                val = cJSON_CreateString(item.strValue());
            }
            else if (type == ItemTypeInt32 || type == ItemTypeFloat32) {
                val = cJSON_CreateNumber(item.fltValue());
            }
            else if (type == ItemTypeForm) {
                auto formString = formIdToString(item.formId());
                val = cJSON_CreateString( formString.c_str() );
            }
            else {
            createNullNode:
                val = cJSON_CreateNull();
            }

            return val;
        }

        static std::string formIdToString(FormId formId) {
            return formIdToString(formId, [](UInt8 modId) {
                DataHandler * dhand = DataHandler::GetSingleton();
                ModInfo * modInfo = dhand->modList.loadedMods[modId];
                return modInfo ? modInfo->name : nullptr;
            });
        }

        static FormId formIdFromString(const char* source) {
            return formIdFromString(source, [](const char *modName) {
                return DataHandler::GetSingleton()->GetModIndex( modName );
            });
        }

        template<class T>
        static std::string formIdToString(FormId formId, T modNameFunc) {

            UInt8 modID = formId >> 24;
            formId = (FormId)(formId & 0x00FFFFFF);

            if (modID == 0xFF)
                return std::string();

            const char * modName = modNameFunc(modID);

            if (!modName) {
                return std::string();
            }

            std::string string = kJSerializedFormData;
            string += kJSerializedFormDataSeparator;
            string += modName;
            string += kJSerializedFormDataSeparator;

            char buff[20] = {'\0'};
            sprintf(buff, "0x%x", formId);
            string += buff;

            return string;
        }

        template<class T>
        static FormId formIdFromString(const char* source, T modIndexFunc) {

            if (!source) {
                return FormZero;
            }

            namespace bs = boost;
            namespace ss = std;

            auto fstring = bs::make_iterator_range(source, source + strnlen_s(source, 1024));

            if (!bs::starts_with(fstring, kJSerializedFormData)) {
                return FormZero;
            }

            ss::vector<decltype(fstring)> substrings;
            bs::split(substrings, source, bs::is_any_of(kJSerializedFormDataSeparator));

            if (substrings.size() != 3) {
                return FormZero;
            }

            auto& pluginName = substrings[1];

            UInt8 modIdx = modIndexFunc( ss::string(pluginName.begin(), pluginName.end()).c_str() );

            if (modIdx == (UInt8)-1) {
                return FormZero;
            }

            auto& formIdString = substrings[2];

            UInt32 formId = 0;
            try {
                formId = std::stoi(ss::string(formIdString.begin(), formIdString.end()), nullptr, 0);
            }
            catch (const std::invalid_argument& ) {
                return FormZero;
            }
            catch (const std::out_of_range& ) {
                return FormZero;
            }

            formId = (modIdx << 24) | (formId & 0x00FFFFFF);

            return (FormId)formId;
        }
    };

}


