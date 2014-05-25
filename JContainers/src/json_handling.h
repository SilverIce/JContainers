#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/find_end.hpp>

#include "skse/GameData.h"

namespace collections {

    inline std::unique_ptr<FILE, decltype(&fclose)> make_unique_file(FILE *file) {
        return std::unique_ptr<FILE, decltype(&fclose)> (file, &fclose);
    }

    template<class T, class D>
    inline std::unique_ptr<T, D> make_unique_ptr(T* data, D destr) {
        return std::unique_ptr<T, D> (data, destr);
    }


    static const char * kJSerializedFormData = "__formData";
    static const char * kJSerializedFormDataSeparator = "|";


    namespace form_parsing {

        template<class T>
        static std::string to_string(FormId formId, T modNameFunc) {

            UInt8 modID = formId >> 24;
            FormId formIdClean = formId;

            const char * modName = nullptr;

            if (modID != FormGlobalPrefix) { // common case
                modName = modNameFunc(modID);
                formIdClean = (FormId)(formId & 0x00FFFFFF);
            }
            else {
                // global form is not bound to any plugin
                modName = "";
            }

            std::string string = kJSerializedFormData;
            string += kJSerializedFormDataSeparator;
            string += modName;
            string += kJSerializedFormDataSeparator;

            char buff[20] = {'\0'};
            sprintf(buff, "0x%x", formIdClean);
            string += buff;

            return string;
        }

        // TODO: rename me!
        template<class T>
        static FormId from_string(const char* source, T modIndexFunc) {

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

            UInt8 modIdx = 0;
            if (!pluginName.empty()) {
                modIdx = modIndexFunc( ss::string(pluginName.begin(), pluginName.end()).c_str() );
                if (modIdx == FormGlobalPrefix) {
                    return FormZero;
                }
            }
            else {
                // 
                modIdx = FormGlobalPrefix;
            }

            auto& formIdString = substrings[2];

            UInt32 formId = 0;
            try {
                formId = std::stoul(ss::string(formIdString.begin(), formIdString.end()), nullptr, 0);
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

        static std::string to_string(FormId formId) {
            return to_string(formId, [](UInt8 modId) {
                DataHandler * dhand = DataHandler::GetSingleton();
                ModInfo * modInfo = dhand->modList.loadedMods[modId];
                return modInfo ? modInfo->name : nullptr;
            });
        }

        // TODO: rename me!
        static FormId from_string(const char* source) {
            return from_string(source, [](const char *modName) {
                return DataHandler::GetSingleton()->GetModIndex( modName );
            });
        }
    }

    class json_deserializer {
        tes_context& _context; 
    public:

        explicit json_deserializer(tes_context & context) : _context(context) {}

        object_base * readJSONFile(const char *path) {
            auto cj = cJSONFromFile(path);
            auto res = readCJSON(cj);
            cJSON_Delete(cj);
            return res;
        }

        object_base * readJSONData(const char *text) {
            if (!text) {
                return nullptr;
            }
            auto cj = cJSON_Parse(text);
            auto res = readCJSON(cj);
            cJSON_Delete(cj);
            return res;
        }

        object_base * readCJSON(cJSON *value) {
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

        cJSON * cJSONFromFile(const char *path) {
            if (!path) {
                return nullptr;
            }

            using namespace std;

            auto file = make_unique_file(fopen(path, "r"));
            if (!file.get())
                return nullptr;

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

        array * makeArray(cJSON *val) {
            array *ar = array::object(_context);

            int count = cJSON_GetArraySize(val);
            for (int i = 0; i < count; ++i) {
            	ar->u_push(makeItem(cJSON_GetArrayItem(val, i)));
            }

            return ar;
        }

        object_base * makeObject(cJSON *val) {

            object_base *object = nullptr;
            bool isFormMap = cJSON_GetObjectItem(val, kJSerializedFormData) != nullptr;

            if (isFormMap) {
                auto obj = form_map::object(_context);
                object = obj;

                int count = cJSON_GetArraySize(val);
                for (int i = 0; i < count; ++i) {
                    auto itm = cJSON_GetArrayItem(val, i);
                    FormId key = form_parsing::from_string(itm->string);

                    if (key) {
                        obj->u_setValueForKey(key, makeItem(itm));
                    }
                }
            } else {

                auto obj = map::object(_context);
                object = obj;

                int count = cJSON_GetArraySize(val);
                for (int i = 0; i < count; ++i) {
                    auto itm = cJSON_GetArrayItem(val, i);
                    obj->u_setValueForKey(itm->string, makeItem(itm));
                }
            }

            return object;
        }

        Item makeItem(cJSON *val) {
            Item item;
            int type = val->type;
            if (type == cJSON_Array) {
                item = makeArray(val);
            } else if (type == cJSON_Object) {
                item = makeObject(val);
            } else if (type == cJSON_String) {

                bool isFormData = strncmp(val->valuestring, kJSerializedFormData, strlen(kJSerializedFormData)) == 0;

                if (!isFormData) {
                    item = val->valuestring;
                } else {
                    item = form_parsing::from_string(val->valuestring);
                }

            } else if (type == cJSON_Number) {
                if (std::floor(val->valuedouble) == val->valuedouble) {
                    item = val->valueint;
                } else {
                    item = val->valuedouble;
                }
            }

            return item;
        }
    };

    class json_serializer {

    public:

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
                    return cJSON_CreateString("<cyclic references in JSON are not supported yet>");
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
                    for (auto& pair : obj->as<map>()->u_container()) {
                         auto cnode = createCJSONNode(pair.second, serializedObjects);
                         if (cnode) {
                             cJSON_AddItemToObject(val, pair.first.c_str(), cnode);
                         }
                    }
                }
                else if (obj->as<form_map>()) {

                    val = cJSON_CreateObject();

                    cJSON_AddItemToObject(val, kJSerializedFormData, cJSON_CreateNull());

                    for (auto& pair : obj->as<form_map>()->u_container()) {
                        auto cnode = createCJSONNode(pair.second, serializedObjects);
                        if (cnode) {
                            auto key = form_parsing::to_string(pair.first);
                            if (!key.empty()) {
                                cJSON_AddItemToObject(val, key.c_str(), cnode);
                            }
                        }
                    }
                }
            }
            else if (type == ItemTypeCString) {

                val = (item.strValue() ? cJSON_CreateString(item.strValue()) : cJSON_CreateNull());
            }
            else if (type == ItemTypeInt32 || type == ItemTypeFloat32) {
                val = cJSON_CreateNumber(item.fltValue());
            }
            else if (type == ItemTypeForm) {
                auto formString = form_parsing::to_string(item.formId());
                val = cJSON_CreateString( formString.c_str() );
            }
            else {
                val = cJSON_CreateNull();
            }

            return val;
        }
    };

}


