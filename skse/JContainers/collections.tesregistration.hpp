#pragma once


#define MESSAGE(...) // _DMESSAGE(__VA_ARGS__);

namespace collections {

    namespace tes_object {
        static const char * TesName() { return "JValue";}

        static HandleT retain(StaticFunctionTag*, HandleT handle) {
            MESSAGE(__FUNCTION__);
            auto obj = collection_registry::getObject(handle);
            if (obj) {
                obj->retain();
                return handle;
            }

            return HandleNull;
        }

        static HandleT autorelease(StaticFunctionTag*, HandleT handle) {
            MESSAGE(__FUNCTION__);
            autorelease_queue::instance().push(handle);
            return handle;
        }

        template<class T>
        static HandleT create(StaticFunctionTag *) {
            MESSAGE(__FUNCTION__);
            return T::create()->id;
        }

        template<class T>
        static HandleT object(StaticFunctionTag *tt) {
            MESSAGE(__FUNCTION__);
            return T::object()->id;
        }

        static void release(StaticFunctionTag*, HandleT handle) {
            MESSAGE(__FUNCTION__);
            object_base *obj = collection_registry::getObject(handle);
            if (obj) {
                obj->release();
            }
        }

        static bool isArray(StaticFunctionTag*, HandleT handle) {
            MESSAGE(__FUNCTION__);
            object_base *obj = collection_registry::getObject(handle);
            return obj && obj->_type == CollectionTypeArray;
        }

        static bool isMap(StaticFunctionTag*, HandleT handle) {
            MESSAGE(__FUNCTION__);
            object_base *obj = collection_registry::getObject(handle);
            return obj && obj->_type == CollectionTypeMap;
        }

        static HandleT readFromFile(StaticFunctionTag*, BSFixedString path) {
            if (path.data == nullptr)
                return 0;

            auto obj = json_parsing::readJSONFile(path.data);
            return  obj ? obj->id : 0;
        }

        static void writeToFile(StaticFunctionTag*, HandleT handle, BSFixedString path) {
            if (path.data == nullptr)  return;

            auto obj = collection_registry::getObject(handle);
            if (!obj)  return;


           
            std::unique_ptr<char, decltype(&free)> data(json_parsing::createJSONData(*obj), &free);
            //char * data = json_parsing::createJSONData(*obj);
            if (!data) return;

            //unique_ptr<FILE, decltype(&fclose)> 
            auto file = make_unique_file(fopen(path.data, "w"));
            if (!file) {
                //free(data);
                return;
            }

            fwrite(data.get(), 1, strlen(data.get()), file.get());
        }

        typedef const char * cstring;

        template<class T>
        static typename Tes2Value<T>::tes_type resolveT(StaticFunctionTag*, HandleT handle, BSFixedString path) {
            auto obj = collection_registry::getObject(handle);
            if (!obj) return 0;

            T val((T)0);
            json_parsing::resolvePath(obj, path.data, [&](Item* itmPtr) {
                if (itmPtr) {
                    val = itmPtr->readAs<T>();
                }
            });

            return typename Tes2Value<T>::tes_type(val);
        }

        static bool hasPath(StaticFunctionTag*, HandleT handle, BSFixedString path) {
            auto obj = collection_registry::getObject(handle);
            if (!obj) return false;

            bool succeed = false;
            json_parsing::resolvePath(obj, path.data, [&](Item* itmPtr) {
                succeed = (itmPtr != nullptr);
            });

            return succeed;
        }

        template<class T>
        static bool solveT(StaticFunctionTag*, HandleT handle, BSFixedString path, typename Tes2Value<T>::tes_type value) {
            auto obj = collection_registry::getObject(handle);
            if (!obj) return false;

            bool succeed = false;
            json_parsing::resolvePath(obj, path.data, [&](Item* itmPtr) {
                if (itmPtr) {
                    val = itmPtr->writeAs(value);
                    succeed = true;
                }
            });

            return succeed;
        }

        void printMethod(const char *cname, const char *cargs) {
            //string args(cargs);

            const char * type2tes[][2] = {
                "HandleT", "int",
                "Index", "int",
                "BSFixedString", "string",
                "Float32", "float",
                "SInt32", "int",
            };
            std::map<string,string> type2tesMap ;
            for (int i = 0; i < sizeof(type2tes) / (2 * sizeof(char*));++i) {
                type2tesMap[type2tes[i][0]] = type2tes[i][1];
            }


            string name;

            vector<string> strings; // #2: Search for tokens
            boost::split( strings, string(cargs), boost::is_any_of(", ") );

            if (strings[0] != "void") {
                name += type2tesMap[strings[0]] + ' ';
            }

            name += "Function ";
            name += cname;
            name += "(";

            int argNum = 0;

            for (int i = 1; i < strings.size(); ++i, ++argNum) {
                if (strings[i].empty()) {
                    continue;
                }


                name += (type2tesMap.find(strings[i]) != type2tesMap.end() ? type2tesMap[strings[i]] : strings[i]);
                name += " arg";
                name += (char)((argNum - 1) + '0');
                if ((i+1) < strings.size())
                    name += ", ";
            }

            name += ") global native\n";

            printf(name.c_str());
        }

#define ARGS(...) __VA_ARGS__

        static bool registerFuncs(VMClassRegistry* registry) {

            #define REGISTER2(name, func, argCount,  ... /*types*/ ) \
                if (registry) { \
                registry->RegisterFunction( \
                new NativeFunction ## argCount <StaticFunctionTag, __VA_ARGS__ >(name, TesName(), func, registry)); \
                registry->SetFunctionFlags(TesName(), name, VMClassRegistry::kFunctionFlag_NoWait); \
                }\
                printMethod(name, #__VA_ARGS__);

            #define REGISTER(func, argCount,  ... /*types*/ ) REGISTER2(#func, func, argCount, __VA_ARGS__)

            REGISTER(release, 1, void, HandleT);
            REGISTER(retain, 1, HandleT, HandleT);
            REGISTER(autorelease, 1, HandleT, HandleT);

            REGISTER(readFromFile, 1, HandleT, BSFixedString);
            REGISTER(writeToFile, 2, void, HandleT, BSFixedString);

            REGISTER(isArray, 1, bool, HandleT);
            REGISTER(isMap, 1, bool, HandleT);

            REGISTER2("resolveVal", resolveT<Handle>, 2, HandleT, HandleT, BSFixedString);
            REGISTER2("resolveFlt", resolveT<Float32>, 2, Float32, HandleT, BSFixedString);
            REGISTER2("resolveStr", resolveT<BSFixedString>, 2, BSFixedString, HandleT, BSFixedString);
            REGISTER2("resolveInt", resolveT<SInt32>, 2, SInt32, HandleT, BSFixedString);

            return true;
        }
    }

    namespace tes_array {
        using namespace tes_object;

        static const char * TesName() { return "JArray";}

        typedef array::Index Index;

        static array* find(HandleT handle) {
            return collection_registry::getObjectOfType<array>(handle);
        }

        template<class T>
        static typename Tes2Value<T>::tes_type itemAtIndex(StaticFunctionTag*, HandleT handle, Index index) {
            MESSAGE(__FUNCTION__);
            auto obj = find(handle);
            if (!obj) {
                return 0;
            }

            mutex_lock g(obj->_mutex);
            return index < obj->_array.size() ? obj->_array[index].readAs<T>() : 0;
        }

        template<class T>
        static void replaceItemAtIndex(StaticFunctionTag*, HandleT handle, Index index, typename Tes2Value<T>::tes_type item) {
            auto obj = find(handle);
            if (!obj) {
                return;
            }

            mutex_lock g(obj->_mutex);
            if (index < obj->_array.size()) {
                obj->_array[index] = Item((T)item);
            }
        }

        static void removeItemAtIndex(StaticFunctionTag*, Handle handle, Index index) {
            auto obj = find(handle);
            if (!obj) {
                return;
            }

            mutex_lock g(obj->_mutex);
            if (index < obj->_array.size()) {
                obj->_array.erase(obj->_array.begin() + index);
            }
        }

        template<class T>
        static void add(StaticFunctionTag*, HandleT handle, typename Tes2Value<T>::tes_type item) {
            MESSAGE(__FUNCTION__);
            print(item);
            auto obj = find(handle);
            if (obj) {
                mutex_lock g(obj->_mutex);
                obj->_array.push_back(Item((T)item));
            }
        }

        static Index count(StaticFunctionTag*, HandleT handle) {
            MESSAGE(__FUNCTION__);
            auto obj = find(handle);
            if (obj) {
                mutex_lock g(obj->_mutex);
                return  obj->_array.size();
            }
            return 0;
        }

        static void clear(StaticFunctionTag*, HandleT handle) {
            auto obj = find(handle);
            if (obj) {
                mutex_lock g(obj->_mutex);
                obj->_array.clear();
            }
        }

        static void eraseIndex(StaticFunctionTag*, HandleT handle, SInt32 index) {
            auto obj = find(handle);
            if (obj) {
                mutex_lock g(obj->_mutex);
                if (index >= 0 && index < obj->_array.size()) {
                    obj->_array.erase(obj->_array.begin() + index);
                }
            }
        }

        static bool registerFuncs(VMClassRegistry* registry) {

            MESSAGE("register array funcs");

            REGISTER2("create", create<array>, 0, HandleT);
            REGISTER2("object", object<array>, 0, HandleT);

            REGISTER(count, 1, Index, HandleT);

            REGISTER2("addFlt", add<Float32>, 2, void, HandleT, Float32);
            REGISTER2("addInt", add<SInt32>, 2, void, HandleT, SInt32);
            REGISTER2("addStr", add<BSFixedString>, 2, void, HandleT, BSFixedString);
            REGISTER2("addVal", add<Handle>, 2, void, HandleT, HandleT);

            REGISTER2("replaceFltAtIndex", replaceItemAtIndex<Float32>, 3, void, HandleT, Index, Float32);
            REGISTER2("replaceIntAtIndex", replaceItemAtIndex<SInt32>, 3, void, HandleT, Index, SInt32);
            REGISTER2("replaceStrAtIndex", replaceItemAtIndex<BSFixedString>, 3, void, HandleT, Index, BSFixedString);
            REGISTER2("replaceValAtIndex", replaceItemAtIndex<Handle>, 3, void, HandleT, Index, HandleT);

            REGISTER2("fltAtIndex", itemAtIndex<Float32>, 2, Float32, HandleT, Index);
            REGISTER2("intAtIndex", itemAtIndex<SInt32>, 2, SInt32, HandleT, Index);
            REGISTER2("strAtIndex", itemAtIndex<BSFixedString>, 2, BSFixedString, HandleT, Index);
            REGISTER2("valAtIndex", itemAtIndex<Handle>, 2, HandleT, HandleT, Index);

            REGISTER2("clear", clear, 1, void, HandleT);
            REGISTER(eraseIndex, 2, void, HandleT, SInt32);

            MESSAGE("funcs registered");

            return true;
        }
    }

    namespace tes_map {
        using namespace tes_object;

        static const char * TesName() { return "JMap";}

        static map* find(HandleT handle) {
            return collection_registry::getObjectOfType<map>(handle);
        }

        template<class T>
        static typename Tes2Value<T>::tes_type getItem(StaticFunctionTag*, HandleT handle, BSFixedString key) {
            MESSAGE(__FUNCTION__);
            auto obj = find(handle);
            if (!obj || key.data == nullptr) {
                return 0;
            }

            mutex_lock g(obj->_mutex);
            auto itr = obj->cnt.find(key.data);
            return itr != obj->cnt.end() ? itr->second.readAs<T>() : 0;
        }

        template<class T>
        static void setItem(StaticFunctionTag*, HandleT handle, BSFixedString key, typename Tes2Value<T>::tes_type item) {
            MESSAGE(__FUNCTION__);
            auto obj = find(handle);
            if (!obj || key.data == nullptr) {
                return;
            }

            mutex_lock g(obj->_mutex);
            obj->cnt[key.data] = Item((T)item);
        }

        static bool hasKey(StaticFunctionTag*, HandleT handle, BSFixedString key) {
            MESSAGE(__FUNCTION__);
            auto obj = find(handle);
            if (!obj || key.data == nullptr) {
                return 0;
            }

            mutex_lock g(obj->_mutex);
            auto itr = obj->cnt.find(key.data);
            return itr != obj->cnt.end();
        }

        static bool removeKey(StaticFunctionTag*, HandleT handle, BSFixedString key) {
            MESSAGE(__FUNCTION__);
            auto obj = find(handle);
            if (!obj || key.data == nullptr) {
                return 0;
            }

            mutex_lock g(obj->_mutex);
            auto itr = obj->cnt.find(key.data);
            bool hasKey = itr != obj->cnt.end();

            obj->cnt.erase(itr);
            return hasKey;
        }

        static SInt32 count(StaticFunctionTag*, HandleT handle) {
            MESSAGE(__FUNCTION__);
            auto obj = find(handle);
            if (!obj) {
                return 0;
            }

            mutex_lock g(obj->_mutex);
            return obj->cnt.size();
        }

        static void clear(StaticFunctionTag*, HandleT handle) {
            MESSAGE(__FUNCTION__);
            auto obj = find(handle);
            if (!obj) {
                return;
            }

            mutex_lock g(obj->_mutex);
            obj->cnt.clear();
        }

        bool registerFuncs(VMClassRegistry* registry) {

            REGISTER2("create", create<array>, 0, HandleT);
            REGISTER2("object", object<array>, 0, HandleT);

            REGISTER(count, 1, SInt32, HandleT);
            REGISTER2("clear", clear, 1, void, HandleT);
            REGISTER(removeKey, 2, bool, HandleT, BSFixedString);

            REGISTER2("setFlt", setItem<Float32>, 3, void, HandleT, BSFixedString, Float32);
            REGISTER2("setInt", setItem<SInt32>, 3, void, HandleT, BSFixedString, SInt32);
            REGISTER2("setStr", setItem<BSFixedString>, 3, void, HandleT, BSFixedString, BSFixedString);
            REGISTER2("setVal", setItem<Handle>, 3, void, HandleT, BSFixedString, HandleT);

            REGISTER2("getFlt", getItem<Float32>, 2, Float32, HandleT, BSFixedString);
            REGISTER2("getInt", getItem<SInt32>, 2, SInt32, HandleT, BSFixedString);
            REGISTER2("getStr", getItem<BSFixedString>, 2, BSFixedString, HandleT, BSFixedString);
            REGISTER2("getVal", getItem<Handle>, 2, HandleT, HandleT, BSFixedString);

            return true;
        }
    }

    namespace tes_db {
        using namespace tes_object;
       // using namespace tes_map;

        static const char * TesName() { return "JDB";}
        
        template<class T>
        static typename Tes2Value<T>::tes_type resolveT(StaticFunctionTag* tag, BSFixedString path) {
            return  tes_map::resolveT<T>(tag, shared_state::instance().databaseId(), path); 
        }

        template<class T>
        static bool solveT(StaticFunctionTag*, BSFixedString path, typename Tes2Value<T>::tes_type value) { 
            auto obj = collection_registry::getObject(shared_state::instance().databaseId());
            if (!obj) return false;

            bool succeed = false;
            json_parsing::resolvePath(obj, path.data, [&](Item* itmPtr) {
                if (itmPtr) {
                    *itmPtr = Item(value);
                    succeed = true;
                }
            });

            return succeed;
        }

        static void setValue(StaticFunctionTag*tag, BSFixedString path, HandleT obj) {
            if (obj) {
                tes_map::setItem<Handle>(tag, shared_state::instance().databaseId(), path, obj);
            } else {
                tes_map::removeKey(tag, shared_state::instance().databaseId(), path);
            }
        }

        static bool hasPath(StaticFunctionTag*tag, BSFixedString path) {
            return tes_object::hasPath(tag, shared_state::instance().databaseId(), path);
        }

        static void writeToFile(StaticFunctionTag* tag, BSFixedString path) {
            tes_object::writeToFile(tag, shared_state::instance().databaseId(), path);
        }

        static void readFromFile(StaticFunctionTag* tag, BSFixedString path) {
            auto objNew = json_parsing::readJSONFile(path.data);
            shared_state::instance().setDataBase(objNew);
        }

        static bool registerFuncs(VMClassRegistry* registry) {

            REGISTER2("setFlt", solveT<Float32>, 2, bool, BSFixedString, Float32);
            REGISTER2("setInt", solveT<SInt32>, 2, bool, BSFixedString, SInt32);
            REGISTER2("setStr", solveT<BSFixedString>, 2, bool, BSFixedString, BSFixedString);
            REGISTER2("setVal", solveT<Handle>, 2, bool, BSFixedString, HandleT);

            REGISTER2("getFlt", resolveT<Float32>, 1, Float32, BSFixedString);
            REGISTER2("getInt", resolveT<SInt32>, 1, SInt32, BSFixedString);
            REGISTER2("getStr", resolveT<BSFixedString>, 1, BSFixedString, BSFixedString);
            REGISTER2("getVal", resolveT<Handle>, 1, HandleT, BSFixedString);

            REGISTER(setValue, 2, void, BSFixedString, HandleT);
            REGISTER(hasPath, 1, bool, BSFixedString);

            REGISTER(writeToFile, 1, void, BSFixedString);
            REGISTER(readFromFile, 1, void, BSFixedString);

            return true;
        }
    }

    void registerFuncs(VMClassRegistry **registryPtr) {
        VMClassRegistry *registry =*registryPtr;

        collections::tes_array::registerFuncs(registry);
        collections::tes_map::registerFuncs(registry);
        collections::tes_object::registerFuncs(registry);
        collections::tes_db::registerFuncs(registry);
    }
}