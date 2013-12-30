#pragma once

#include "collections.h"
#include "cJSON.h"

namespace collections {

    class json_parsing {
    public:

        static Item resolvePath(collection_base *collection, const char *path) {
            const char *c = path;

            Item itm(collection);

            collection_base *id = nullptr;

            while (*c ) {

                char s = *c++;

                id = itm.objValue();

                if (s == '.') {

                    if (!id || !id->as<map>()) {
                         return Item();
                    }

                    char buff[256] = {'\0'};
                    char *buffPtr = buff;

                    while (*c && *c != '.' && *c != '[') {
                        if (*c == ']') {
                            return Item();
                        }
                        *buffPtr++ = *c++;
                    }
                    *buffPtr = '\0';

                    auto& cnt = id->as<map>()->cnt;
                    auto itr = cnt.find(buff);
                    if (itr != cnt.end()) {
                        itm = itr->second;
                    } else {
                        return Item();
                    }
                }
                else if (s == '[') {
                    if (!id || !id->as<array>()) {
                        return Item();
                    }

                    if (*c == ']') {
                        return Item();
                    }

                    int num = 0;
                    while (*c != ']') {
                        if (!*c || 
                            !(*c >= '0' && *c <= '9')) {
                                return Item();
                        }
                        num = num * 10 + (*c - '0');
                        ++c;
                    }
                    //c is ] now

                    auto& arr = id->as<array>()->_array;
                    if (num < arr.size()) {
                        itm = arr[num];
                    } else {
                        return Item();
                    }
                }
            }

            return itm;
        }

        static collection_base * readJSONFile(const char *path) {
            auto cj = cJSONFromFile(path);
            auto res = readCJSON(cj);
            cJSON_Delete(cj);
            return res;
        }

        static collection_base * readJSONData(const char *text) {
            auto cj = cJSON_Parse(text);
            auto res = readCJSON(cj);
            cJSON_Delete(cj);
            return res;
        }

        static collection_base * readCJSON(cJSON *value) {
            if (!value) {
                return nullptr;
            }

            collection_base *obj = nullptr;
            if (value->type == cJSON_Array || value->type == cJSON_Object) {
                Item itm = makeItem(value);
                obj = itm.objValue();
            }

            return obj;
        }

        static cJSON * cJSONFromFile(const char *path) {
            FILE *file = fopen(path, "r");
            if (!file)
                return 0;

            char buffer[1024];
            std::vector<char> bytes;
            size_t readen = 0;
            while (!ferror(file) && !feof(file)) {
                readen = fread(buffer, 1, sizeof(buffer), file);
                if (readen > 0) {
                    bytes.insert(bytes.end(), buffer, buffer + readen);
                }
                else  {
                    break;
                }
            }
            bytes.push_back(0);
            fclose(file);

            return cJSON_Parse(&bytes[0]);
        }

        static  array * makeArray(cJSON *val) {
            array *ar = array::object();

            int count = cJSON_GetArraySize(val);
            for (int i = 0; i < count; ++i) {
            	ar->push(makeItem(cJSON_GetArrayItem(val, i)));
            }

            return ar;
        }

        static map * nmakeObject(cJSON *val) {
            auto ar = map::object();

            int count = cJSON_GetArraySize(val);
            for (int i = 0; i < count; ++i) {
                auto itm = cJSON_GetArrayItem(val, i);
                (*ar)[itm->string] = makeItem(itm);
            }

            return ar;
        }

        static Item makeItem(cJSON *val) {
            Item item;
            int type = val->type;
            if (type == cJSON_Array) {
                auto ar = makeArray(val);
                item.setObjectVal(ar);
            } else if (type == cJSON_Object) {
                auto ar = nmakeObject(val);
                item.setObjectVal(ar);
            } else if (type == cJSON_String) {
                item.setStringVal(val->valuestring);
            } else if (type == cJSON_Number) {
                if (std::floor(val->valuedouble) == val->valuedouble) {
                    item.setInt(val->valueint);
                } else {
                    item.setFlt(val->valuedouble);
                }
            }

            return item;
        }


        static cJSON * createCJSON(collection_base & collection) {
            return createCJSONNode(Item(&collection));
        }

        static char * createJSONData(collection_base & collection) {
            auto node = createCJSONNode(Item(&collection));
            char *data = cJSON_Print(node);
            cJSON_Delete(node);
            return data;
        }

        static cJSON * createCJSONNode(const Item& item) {

            cJSON *val = nullptr;

            ItemType type = item.type();

            if (type == ItemTypeObject && item.objValue()) {
                auto obj = item.objValue();
                mutex_lock g(obj->_mutex);
                if (obj->as<array>()) {
                    val = cJSON_CreateArray();
                    array *ar = obj->as<array>();
                    for (auto itm : ar->_array) {
                        cJSON_AddItemToArray(val, createCJSONNode(itm));
                    }

                } else if (obj->as<map>()) {
                    val = cJSON_CreateObject();
                    for (auto pair : obj->as<map>()->cnt) {
                        cJSON_AddItemToObject(val, pair.first.c_str(), createCJSONNode(pair.second));
                    }
                }
            }
            else if (type == ItemTypeCString) {
                val = cJSON_CreateString(item.strValue());
            } else if (type == ItemTypeInt32 || type == ItemTypeFloat32) {
                val = cJSON_CreateNumber(item.fltValue());
            }

            return val;
        }
    };
}


