#pragma once

#include "collections.h"
#include "cJSON.h"

namespace collections {

    std::unique_ptr<FILE, decltype(&fclose)> make_unique_file(FILE *file) {
        return std::unique_ptr<FILE, decltype(&fclose)> (file, &fclose);
    }

    class json_parsing {
    public:

        class collection_owner {
            object_base *_collection;
        public:
            explicit collection_owner(object_base *coll) {
                setCollection(coll);
            }

            ~collection_owner() {
                setCollection(nullptr);
            }

            object_base* operator -> () const {
                return _collection;
            }

            object_base* get () const {
                return _collection;
            }

            bool operator !() const {
                return _collection == nullptr;
            }

            void setCollection(object_base *coll) {
                if (_collection != coll) {
                    if (_collection) {
                        _collection->_mutex.unlock();
                        _collection->release();
                    }
                    _collection = coll;
                    if (coll) {
                        coll->retain();
                        coll->_mutex.lock();
                    }
                }
            }
        };

        static Item resolvePath(object_base *collection, const char *path) {
            const char *c = path;

            Item itm;

    /*        auto unlocker = [=](collection_base *obj) {
                if (obj) {
                    obj->_mutex.unlock();
                }
            };

            std::unique_ptr<collection_base, decltype(unlocker)> objOwner(collection, unlocker);*/

            collection_owner id (collection);

            while (*c ) {

                char s = *c++;

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

                        if (!itr->second.objValue())
                            itm = itr->second;

                        id.setCollection(itr->second.objValue());

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
                        if (!arr[num].objValue())
                            itm = arr[num];

                        id.setCollection(arr[num].objValue());
                    } else {
                        return Item();
                    }
                }
            }

            object_base *obj = id.get();
            id.setCollection(nullptr);

            return obj ? Item(obj) : itm;
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
                obj = itm.objValue();
            }

            return obj;
        }

        static cJSON * cJSONFromFile(const char *path) {
            using namespace std;

            auto file = make_unique_file(fopen(path, "r"));
           // FILE *file = fopen(path, "r");
            if (!file.get())
                return 0;

            char buffer[1024];
            std::vector<char> bytes;
            size_t readen = 0;
            while (!ferror(file.get()) && !feof(file.get())) {
                readen = fread(buffer, 1, sizeof(buffer), file.get());
                if (readen > 0) {
                    bytes.insert(bytes.end(), buffer, buffer + readen);
                }
                else  {
                    break;
                }
            }
            bytes.push_back(0);
            //fclose(file);

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


        static cJSON * createCJSON(object_base & collection) {
            return createCJSONNode(Item(&collection));
        }

        static char * createJSONData(object_base & collection) {
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


