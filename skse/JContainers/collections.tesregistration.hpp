#pragma once

#include "tes_binding.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#define MESSAGE(...) // _DMESSAGE(__VA_ARGS__);

namespace collections {

    const char *kCommentObject = "creates new container object. returns container identifier (integral number).\n"
                                 "identifier is the thing you will have to pass to the most of container's functions as first argument";

    class tes_object : public tes_binding::class_meta_mixin< tes_object > {
    public:

        REGISTER_TES_NAME("JValue");

        static const char * TesName() { return "JValue";}

        static object_base* retain(object_base *obj) {
            if (obj) {
                obj->tes_retain();
                return obj;
            }

            return nullptr;
        }
        REGISTERF2(retain, "*",
            "Retains and returns the object.\n\
            All containers that created with object* or objectWith* methods get automatically destroyed after some amount of time (~10 seconds)\n\
            To keep object alive you must retain in once and you have to __release__ it when you do not need it anymore (also to not pollute save file).\n\
            An alternative to retain-release is store object in JDB container"
        );

        static HandleT autorelease(HandleT handle) {
            MESSAGE(__FUNCTION__);
            autorelease_queue::instance().push(handle);
            return handle;
        }

        template<class T>
        static T* create() {
            MESSAGE(__FUNCTION__);
            return T::create();
        }

        template<class T>
        static object_base* object() {
            MESSAGE(__FUNCTION__);
            return T::object();
        }

        static void release(object_base *obj) {
            MESSAGE(__FUNCTION__);
            if (obj) {
                obj->tes_release();
            }
        }
        REGISTERF2(release, "*", "releases the object");

        static bool isArray(object_base *obj) {
            return obj && obj->_type == CollectionTypeArray;
        }
        REGISTERF2(isArray, "*", "returns true if object is map, array or formmap container");

        static bool isMap(object_base *obj) {
            return obj && obj->_type == CollectionTypeMap;
        }
        REGISTERF2(isMap, "*", NULL);

        static bool isFormMap(object_base *obj) {
            return obj && obj->as<form_map>();
        }
        REGISTERF2(isFormMap, "*", NULL);

        static SInt32 count(object_base *obj) {
            return obj ? obj->s_count() : 0;
        }
        //REGISTERF2(count, "*", "items count");

        static void clear(object_base *obj) {
            if (obj) {
                obj->s_clear();
            }
        }
        //REGISTERF2(clear, "*", "remove all items");

        static object_base* readFromFile(const char *path) {
            if (path == nullptr)
                return 0;

            auto obj = json_parsing::readJSONFile(path);
            return  obj;
        }
        REGISTERF2(readFromFile, "* filePath", "parse JSON and create a new object (array or map)");

        static object_base* objectFromPrototype(const char *prototype) {
            if (!prototype)
                return nullptr;

            auto obj = json_parsing::readJSONData(prototype);
            return obj;
        }
        //REGISTERF2(objectFromPrototype, "* prototype", "creates new container object using given string-prototype");

        static void writeToFile(object_base *obj, const char * path) {
            if (path == nullptr)  return;
            if (!obj)  return;

            std::unique_ptr<char, decltype(&free)> data(json_parsing::createJSONData(*obj), &free);
            //char * data = json_parsing::createJSONData(*obj);
            if (!data) return;

            //unique_ptr<FILE, decltype(&fclose)> 
            auto file = make_unique_file(fopen(path, "w"));
            if (!file) {
                //free(data);
                return;
            }

            fwrite(data.get(), 1, strlen(data.get()), file.get());
        }
        REGISTERF2(writeToFile, "* filePath", "write object into JSON file");

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
                    itmPtr->writeAs(value);
                    succeed = true;
                }
            });

            return succeed;
        }
#define ARGS(...) __VA_ARGS__

        static bool registerFuncs(VMClassRegistry* registry) {

            #define REGISTER2(name, func, argCount,  ... /*types*/ ) \
                if (registry) { \
                registry->RegisterFunction( \
                new NativeFunction ## argCount <StaticFunctionTag, __VA_ARGS__ >(name, TesName(), func, registry)); \
                registry->SetFunctionFlags(TesName(), name, VMClassRegistry::kFunctionFlag_NoWait); \
                }
                //printMethod(name, #__VA_ARGS__);

            #define REGISTER(func, argCount,  ... /*types*/ ) REGISTER2(#func, func, argCount, __VA_ARGS__)

            bind(registry);

            return true;
        }
    };

    class tes_array : public tes_binding::class_meta_mixin< tes_array > {
    public:

        REGISTER_TES_NAME("JArray");

        static void additionalSetup() {
            metaInfo().extendsClass = "JValue";
        }

        static const char * TesName() { return "JArray";}

        typedef array::Index Index;

        static array* find(HandleT handle) {
            return collection_registry::getObjectOfType<array>(handle);
        }

        REGISTERF(tes_object::object<array>, "object", "", kCommentObject);

        static object_base* objectWithSize(UInt32 size) {
            auto obj = array::objectWithInitializer([&](array *me) {
                me->_array.resize(size);
            });

            return obj;
        }
        REGISTERF2(objectWithSize, "size", "creates array of given size, filled with nothing");

        template<class T>
        static object_base* fromArray(VMArray<T> arr) {
            auto obj = array::objectWithInitializer([&](array *me) {
                for (UInt32 i = 0; i < arr.Length(); ++i) {
                    T val;
                    arr.Get(&val, i);
                    me->_array.push_back(Item(val));
                }
            });

            return obj;
        }
        REGISTERF(fromArray<SInt32>, "objectWithInts", "values", "creates new array that contains given values");
        REGISTERF(fromArray<BSFixedString>, "objectWithStrings",  "values", NULL);
        REGISTERF(fromArray<Float32>, "objectWithFloats",  "values", NULL);
        REGISTERF(fromArray<bool>, "objectWithBooleans",  "values", NULL);

        template<class T>
        static T itemAtIndex(array *obj, Index index) {
            if (!obj) {
                return T(0);
            }

            mutex_lock g(obj->_mutex);
            return (index >= 0 && index < obj->_array.size()) ? obj->_array[index].readAs<T>() : T(0);
        }
        REGISTERF(itemAtIndex<SInt32>, "getInt", "* index", "returns value at index");
        REGISTERF(itemAtIndex<Float32>, "getFlt", "* index", "");
        REGISTERF(itemAtIndex<const char *>, "getStr", "* index", "");
        REGISTERF(itemAtIndex<Handle>, "getObj", "* index", "");
        REGISTERF(itemAtIndex<TESForm*>, "getForm", "* index", "");

        template<class T>
        static SInt32 findVal(array *obj, T value) {
            if (!obj) {
                return -1;
            }

            mutex_lock g(obj->_mutex);

            auto itr = std::find_if(obj->_array.begin(), obj->_array.end(), [=](const Item& item) {
                return item.isEqual(value);
            });

            return itr != obj->_array.end() ? (itr - obj->_array.begin()) : -1;
        }
        REGISTERF(findVal<SInt32>, "findInt", "* value", "returns index of the first found value that equals to given value.\n\
            if found nothing returns -1.");
        REGISTERF(findVal<Float32>, "findFlt", "* value", "");
        REGISTERF(findVal<const char *>, "findStr", "* value", "");
        REGISTERF(findVal<object_base*>, "findObj", "* value", "");
        REGISTERF(findVal<TESForm*>, "findForm", "* value", "");

        template<class T>
        static void replaceItemAtIndex(array *obj, Index index, T item) {
            if (!obj) {
                return;
            }

            mutex_lock g(obj->_mutex);
            if (index >= 0 && index < obj->_array.size()) {
                obj->_array[index] = Item(item);
            }
        }
        REGISTERF(replaceItemAtIndex<SInt32>, "setInt", "* index value", "replaces existing value at index with new value");
        REGISTERF(replaceItemAtIndex<Float32>, "setFlt", "* index value", "");
        REGISTERF(replaceItemAtIndex<const char *>, "setStr", "* index value", "");
        REGISTERF(replaceItemAtIndex<object_base*>, "setObj", "* index value", "");
        REGISTERF(replaceItemAtIndex<TESForm*>, "setForm", "* index value", "");

        template<class T>
        static void add(array *obj, T item) {
            MESSAGE(__FUNCTION__);
            if (obj) {
                mutex_lock g(obj->_mutex);
                obj->_array.push_back(Item(item));
            }
        }
        REGISTERF(add<SInt32>, "addInt", "* value", "appends value to the end of array");
        REGISTERF(add<Float32>, "addFlt", "* value", "");
        REGISTERF(add<const char *>, "addStr", "* value", "");
        REGISTERF(add<object_base*>, "addObj", "* value", "");
        REGISTERF(add<TESForm*>, "addForm", "* value", "");

        static Index count(array *obj) {
            MESSAGE(__FUNCTION__);
            if (obj) {
                mutex_lock g(obj->_mutex);
                return  obj->_array.size();
            }
            return 0;
        }
        REGISTERF2(count, "*", "inserted items count");

        static void clear(array *obj) {
            if (obj) {
                mutex_lock g(obj->_mutex);
                obj->_array.clear();
            }
        }
        REGISTERF2(clear, "*", "remove all items from array");

        static void eraseIndex(array *obj, SInt32 index) {
            if (obj) {
                mutex_lock g(obj->_mutex);
                if (index >= 0 && index < obj->_array.size()) {
                    obj->_array.erase(obj->_array.begin() + index);
                }
            }
        }
        REGISTERF2(eraseIndex, "* index", "erases item at index");

        static bool registerFuncs(VMClassRegistry* registry) {
            MESSAGE("register array funcs");


            bind(registry);

            return true;
        }
    };

    inline const char* tes_hash(const char* in) {
        return in;
    }

    inline FormId tes_hash(TESForm * in) {
        return (FormId) (in ? in->formID : 0); 
    }

    template<class Key, class Cnt>
    class tes_map_t : public tes_binding::class_meta_mixin< tes_map_t<Key, Cnt> > {
    public:

        REGISTER_TES_NAME("tt");

        //REGISTERF(create<Cnt>, "create", "", "");
        REGISTERF(tes_object::object<Cnt>, "object", "", kCommentObject);

        template<class T>
        static T getItem(Cnt *obj, Key key) {
            if (!obj || !key) {
                return T(0);
            }

            mutex_lock g(obj->_mutex);
            auto item = obj->find(tes_hash(key));
            return item ? item->readAs<T>() : T(0);
        }
        REGISTERF(getItem<SInt32>, "getInt", "object key", "returns value associated with key");
        REGISTERF(getItem<Float32>, "getFlt", "object key", "");
        REGISTERF(getItem<const char *>, "getStr", "object key", "");
        REGISTERF(getItem<Handle>, "getObj", "object key", "");
        REGISTERF(getItem<TESForm*>, "getForm", "object key", "");

        template<class T>
        static void setItem(Cnt *obj, Key key, T item) {
            if (!obj || !key) {
                return;
            }

            mutex_lock g(obj->_mutex);
            (*obj)[tes_hash(key)] = Item((T)item);
        }
        REGISTERF(setItem<SInt32>, "setInt", "* key", "creates key-value association. replaces existing value if any");
        REGISTERF(setItem<Float32>, "setFlt", "* key", "");
        REGISTERF(setItem<const char *>, "setStr", "* key", "");
        REGISTERF(setItem<object_base*>, "setObj", "* key object2", "");
        REGISTERF(setItem<TESForm*>, "setForm", "* key", "");

        static bool hasKey(Cnt *obj, Key key) {
            if (!obj || !key) {
                return 0;
            }

            mutex_lock g(obj->_mutex);
            auto item = obj->find(tes_hash(key));
            return item != nullptr;
        }
        REGISTERF2(hasKey, "* key", "true, if something associated with key");

        static object_base* allKeys(Cnt *obj) {
            if (!obj) {
                return nullptr;
            }

            return array::objectWithInitializer([=](array *arr) {
                mutex_lock g(obj->_mutex);

                arr->_array.reserve( obj->u_count() );
                for each(auto& pair in obj->container()) {
                    arr->u_push( Item(pair.first) );
                }
            });
        }
        REGISTERF2(allKeys, "*", "returns new array containing all keys");

        static object_base* allValues(Cnt *obj) {
            if (!obj) {
                return nullptr;
            }

            return array::objectWithInitializer([=](array *arr) {
                mutex_lock g(obj->_mutex);

                arr->_array.reserve( obj->u_count() );
                for each(auto& pair in obj->container()) {
                    arr->_array.push_back( pair.second );
                }
            });
        }
        REGISTERF2(allValues, "*", "returns new array containing all values");

        static bool removeKey(Cnt *obj, Key key) {
            if (!obj || !key) {
                return 0;
            }

            mutex_lock g(obj->_mutex);
            return obj->erase(tes_hash(key));
        }
        REGISTERF2(removeKey, "* key", "destroys key-value association");

        static SInt32 count(Cnt *obj) {
            if (!obj) {
                return 0;
            }

            return obj->s_count();
        }
        REGISTERF2(count, "*", "count of items/associations");

        static void clear(Cnt *obj) {
            if (!obj) {
                return;
            }

            obj->s_clear();
        }
        REGISTERF2(clear, "*", "remove all items from map container");

        static bool registerFuncs(VMClassRegistry* registry) {
            bind(registry);
            return true;
        }

        static void additionalSetup();
    };

    typedef tes_map_t<const char*, map > tes_map;
    typedef tes_map_t<TESForm *, form_map> tes_form_map;

    void tes_map::additionalSetup() {
        metaInfo().className = "JMap";
        metaInfo().extendsClass = "JValue";
    }

    void tes_form_map::additionalSetup() {
        metaInfo().className = "JFormMap";
        metaInfo().extendsClass = "JValue";
    }

    class tes_db : public tes_binding::class_meta_mixin<tes_db> {
    public:

        REGISTER_TES_NAME("JDB");

        static void additionalSetup() {}

        static const char * TesName() { return "JDB";}
        
        template<class T>
        static typename Tes2Value<T>::tes_type solveGetter(StaticFunctionTag* tag, BSFixedString path) {
            return tes_object::resolveT<T>(tag, shared_state::instance().databaseId(), path); 
        }

        template<class T>
        static bool solveSetter(StaticFunctionTag*, BSFixedString path, typename Tes2Value<T>::tes_type value) { 
            auto obj = collection_registry::getObject(shared_state::instance().databaseId());
            if (!obj) return false;

            bool succeed = false;
            json_parsing::resolvePath(obj, path.data, [&](Item* itmPtr) {
                if (itmPtr) {
                    *itmPtr = Item((T)value);
                    succeed = true;
                }
            });

            return succeed;
        }

        static void setValue(const char *path, object_base *obj) {
            object_base *db = shared_state::instance().database();
            map *dbMap = db ? db->as<map>() : nullptr;

            if (!dbMap) {
                return;
            }

            if (obj) {
                tes_map::setItem(dbMap, path, obj);
            } else {
                tes_map::removeKey(dbMap, path);
            }
        }
        REGISTERF(setValue, "setObj", "key object", "");

        static bool hasPath(StaticFunctionTag*tag, BSFixedString path) {
            return tes_object::hasPath(tag, shared_state::instance().databaseId(), path);
        }

        static object_base* allKeys() {
            return tes_map::allKeys( shared_state::instance().database() );
        }
        REGISTERF2(allKeys, "*", "returns new array containing all keys");

        static object_base* allValues() {
            return tes_map::allValues( shared_state::instance().database() );
        }
        REGISTERF2(allValues, "*", "returns new array containing all containers associated with JDB");

        static void writeToFile(const char * path) {
            tes_object::writeToFile( shared_state::instance().database(), path);
        }
        REGISTERF2(writeToFile, "path", "writes storage data into JSON file");

        static void readFromFile(/*StaticFunctionTag* tag,*/ const char *path) {
            auto objNew = json_parsing::readJSONFile(path);
            shared_state::instance().setDataBase(objNew);
        }
        REGISTERF2(readFromFile, "path", "fills storage with JSON data");

        static bool registerFuncs(VMClassRegistry* registry) {

            REGISTER2("solveFltSetter", solveSetter<Float32>, 2, bool, BSFixedString, Float32);
            REGISTER2("solveIntSetter", solveSetter<SInt32>, 2, bool, BSFixedString, SInt32);
            REGISTER2("solveStrSetter", solveSetter<BSFixedString>, 2, bool, BSFixedString, BSFixedString);
            REGISTER2("solveObjSetter", solveSetter<Handle>, 2, bool, BSFixedString, HandleT);
            REGISTER2("solveFormSetter", solveSetter<TESForm*>, 2, bool, BSFixedString, TESForm*);

            REGISTER2("solveFlt", solveGetter<Float32>, 1, Float32, BSFixedString);
            REGISTER2("solveInt", solveGetter<SInt32>, 1, SInt32, BSFixedString);
            REGISTER2("solveStr", solveGetter<BSFixedString>, 1, BSFixedString, BSFixedString);
            REGISTER2("solveObj", solveGetter<Handle>, 1, HandleT, BSFixedString);
            REGISTER2("solveForm", solveGetter<TESForm*>, 1, TESForm*, BSFixedString);

            REGISTER(hasPath, 1, bool, BSFixedString);

            bind(registry);

            return true;
        }
    };

    class tes_jcontainers : public tes_binding::class_meta_mixin<tes_jcontainers> {
    public:

        REGISTER_TES_NAME("JContainers");

        static void additionalSetup() {
            metaInfo().comment = "Various utility methods";
        }

        static bool isInstalled() {
            return true;
        }
        REGISTERF2(isInstalled, NULL, "returns true if JContainers plugin is installed");

        static bool fileExistsAtPath(const char *filename) {
            struct _stat buf;
            // Get data associated with "crt_stat.c": 
            int result = _stat(filename, &buf);
            return result == 0;
        }
        REGISTERF2(fileExistsAtPath, "path", "returns true if file at path exists");
    };

    bool registerFuncs(VMClassRegistry *registry) {
        collections::tes_array::registerFuncs(registry);

        collections::tes_map::registerFuncs(registry);
        collections::tes_form_map::registerFuncs(registry);

        collections::tes_object::registerFuncs(registry);

        collections::tes_db::registerFuncs(registry);

        collections::tes_jcontainers::bind(registry);

        return true;
    }

    void registerFuncsHook(VMClassRegistry **registryPtr) {
        registerFuncs(*registryPtr);
    }
}
