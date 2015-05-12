
#include "lua_module.h"

namespace tes_api_3 {

    using namespace collections;

    const char *kCommentObject = "creates new container object. returns container identifier (integer number).\n"
        "identifier is the thing you will have to pass to the most of container's functions as a first argument";

#define VALUE_TYPE_COMMENT "0 - no value, 1 - none, 2 - int, 3 - float, 4 - form, 5 - object, 6 - string"

    class tes_object : public class_meta< tes_object > {
    public:

        typedef object_base* ref;

        REGISTER_TES_NAME("JValue");

        void additionalSetup() {
            metaInfo.comment = "Each container (JArray, JMap & JFormMap) inherits JValue functionality";
        }

        static object_base* retain(ref obj, const char* tag = nullptr) {
            if (obj) {
                obj->tes_retain();
                obj->set_tag(tag);
                return obj;
            }
         
            return nullptr;
        }
        REGISTERF2(retain, "* tag=\"\"",
R"===(--- Lifetime management functionality.
Read this https://github.com/SilverIce/JContainers/wiki/Lifetime-Management before using any of lifetime management functions

Retains and returns the object. Purpose - extend object lifetime)==="
            );

        template<class T>
        static T* object() {
            return &T::object(tes_context::instance());
        }

        static object_base* release(ref obj) {
            if (obj) {
                obj->tes_release();
            }

            return nullptr;
        }
        REGISTERF2(release, "*", "Releases the object and returns zero, so you can release and nullify with one line of code: object = JValue.release(object)");

        static object_base* releaseAndRetain(ref previousObject, ref newObject, const char* tag = nullptr) {
            if (previousObject != newObject) {
                if (previousObject) {
                    previousObject->tes_release();
                }

                retain(newObject, tag);
            }

            return newObject;
        }
        REGISTERF2(releaseAndRetain, "previousObject newObject tag=\"\"",
"Just a union of retain-release calls. Releases previousObject, retains and returns newObject.");

        static void releaseObjectsWithTag(const char *tag) {
            if (!tag) {
                return;
            }

            auto objects = tes_context::instance().filter_objects([tag](const object_base& obj) {
                return obj.has_equal_tag(tag);
            });

            for (auto& ref : objects) {
                while (ref->_tes_refCount != 0) {
                    ref->tes_release();
                }
            }
        }
        REGISTERF2(releaseObjectsWithTag, "tag",
"For cleanup purpose only - releases lost (and not lost) objects with given tag.\n"
"Complements all retain calls objects with given tag received with release calls.\n"
"See 'object lifetime management' section for more information");

        static ref zeroLifetime(ref obj) {
            if (obj) {
                obj->zero_lifetime();
            }
            return obj;
        }
        REGISTERF2(zeroLifetime, "*", "Reduces the time JC temporarily owns the object to a minimal value, returns the object.\n\
By using this function a user helps JC to get rid of no-more-needed to the user object as soon as possible (ofc. the object won't be deleted if something retains or contains it)");

#       define JC_OBJECT_POOL_KEY   "__tempPools"

        static object_base* addToPool(ref obj, const char *poolName) {
            if (poolName) {
                std::string path("." JC_OBJECT_POOL_KEY ".");
                path += poolName;

                array::ref location;

                ca::visit_value(*tes_context::instance().database(), path.c_str(), ca::creative, [&](item& value) {
                    if (auto loc = value.object()->as<array>()) {
                        location = loc;
                    }
                    else {
                        location = &array::object(tes_context::instance());
                        value = location.get();
                    }
                });

/*
                path_resolving::resolve(tes_context::instance(), tes_context::instance().database(), path.c_str(), [&](item* itmPtr) {
                    if (itmPtr) {
                        if (auto loc = itmPtr->object()->as<array>()) {
                            location = loc;
                        }
                        else {
                            location = &array::object(tes_context::instance());
                            *itmPtr = location.get();
                        }
                    }
                },
                    true);*/

                if (location) {
                    location->push(item(obj));
                }
            }

            return obj;
        }
        REGISTERF2(addToPool, "* poolName",
"Handly for temporary objects (objects with no owners) - pool 'locationName' owns any amount of objects, preventing their destuction, extends lifetime.\n\
Do not forget to clean location later! Typical use:\n\
int tempMap = JValue.addToPool(JMap.object(), \"uniquePoolName\")\n\
anywhere later:\n\
JValue.cleanPool(\"uniquePoolName\")"
);

        static void cleanPool(const char *poolName) {
            if (poolName) {
                auto locationsMap = tes_context::instance().database()->findOrDef(JC_OBJECT_POOL_KEY).object()->as<map>();
                if (locationsMap) {
                    locationsMap->erase(poolName);
                }
            }
        }
        REGISTERF2(cleanPool, "poolName", nullptr);

        static ref shallowCopy(ref obj) {
            return obj ? &deep_copying::shallow_copy(tes_context::instance(), *obj) : nullptr;
        }
        REGISTERF2(shallowCopy, "*", "--- Mics. functionality\n\nReturns shallow copy (doesn't copy child objects)");

        static ref deepCopy(ref obj) {
            return obj ? &deep_copying::deep_copy(tes_context::instance(), *obj) : nullptr;
        }
        REGISTERF2(deepCopy, "*", "Returns deep copy");

        static bool isExists(ref obj) {
            return obj != nullptr;
        }
        REGISTERF2(isExists, "*", "ntests whether given object identifier points to existing object");

        template<class T> static bool isCast(ref obj) {
            return obj->as<T>() != nullptr;
        }

        REGISTERF(isCast<array>, "isArray", "*", "returns true if object is map, array or formmap container");
        REGISTERF(isCast<map>, "isMap", "*", nullptr);
        REGISTERF(isCast<form_map>, "isFormMap", "*", nullptr);
        REGISTERF(isCast<integer_map>, "isIntegerMap", "*", nullptr);

        static bool empty(ref obj) {
            return count(obj) == 0;
        }
        REGISTERF2(empty, "*", "returns true, if container is empty");

        static SInt32 count(ref obj) {
            return obj ? obj->s_count() : 0;
        }
        REGISTERF2(count, "*", "returns the number of items in container");

        static void clear(ref obj) {
            if (obj) {
                obj->s_clear();
            }
        }
        REGISTERF2(clear, "*", "removes all items from container");

        static object_base* readFromFile(const char *path) {
            auto obj = json_deserializer::object_from_file(tes_context::instance(), path);
            return  obj;
        }
        REGISTERF2(readFromFile, "filePath", "JSON serialization/deserialization:\n\ncreates and returns new container object containing the contents of JSON file");

        static object_base* readFromDirectory(const char *dirPath, const char *extension = "")
        {
            using namespace boost;

            if (!dirPath || !filesystem::exists( dirPath )) {
                return nullptr;
            }

            if (!extension) {
                extension = "";
            }

            filesystem::directory_iterator end_itr;
            filesystem::path root(dirPath);

            map &files = map::object(tes_context::instance()); 

            for ( filesystem::directory_iterator itr( root ); itr != end_itr; ++itr ) {

                if ( filesystem::is_regular_file( *itr ) &&
                     (!*extension || itr->path().extension().generic_string().compare(extension) == 0) ) {
                    auto asniString = itr->path().generic_string();
                    auto jsonObject = tes_object::readFromFile(asniString.c_str());

                    if (jsonObject) {
                        files.set(itr->path().filename().generic_string(), item(jsonObject));
                    }  
                }
            }

            return &files;
        }
        REGISTERF2(readFromDirectory, "directoryPath extension=\"\"",
            "parses JSON files in directory (non recursive) and returns JMap containing {filename, container-object} pairs.\n"
            "note: by default it does not filters files by extension and will try to parse everything");

        static object_base* objectFromPrototype(const char *prototype) {
            auto obj = json_deserializer::object_from_json_data( tes_context::instance(), prototype);
            return obj;
        }
        REGISTERF2(objectFromPrototype, "prototype", "creates new container object using given JSON string-prototype");

        static void writeToFile(object_base *obj, const char * cpath) {
            if (!cpath || !obj) {
                return;
            }

            boost::filesystem::path path(cpath);
            auto& dir = path.remove_filename();
            if (!dir.empty() && !boost::filesystem::exists(dir) &&
                (boost::filesystem::create_directories(dir), !boost::filesystem::exists(dir)))
            {
                return;
            }

            auto json = json_serializer::create_json_value(*obj);
            if (json) {
                json_dump_file(json.get(), cpath, JSON_INDENT(2));
            }
        }
        REGISTERF(writeToFile, "writeToFile", "* filePath", "writes object into JSON file");

        static SInt32 solvedValueType(object_base* obj, const char *path) {
            SInt32 type = item_type::no_item;

            if (obj && path) {
                ca::visit_value(*obj, path, ca::constant, [&](const item& value) {
                    type = value.type();
                });
            }

            return type;
        }

        static bool hasPath(object_base* obj, const char *path) {
            return solvedValueType(obj, path) != item_type::no_item;
        }
        REGISTERF(hasPath, "hasPath", "* path",
"Path resolving:\n\n\
returns true, if container capable resolve given path.\n\
for ex. JValue.hasPath(container, \".player.health\") will check if given container has 'player' which has 'health' information"
                                      );

        REGISTERF(solvedValueType, "solvedValueType", "* path", "Returns type of resolved value. "VALUE_TYPE_COMMENT);

        template<class T>
        static T resolveGetter(object_base *obj, const char* path, T val = T(0)) {
            if (!obj || !path)
                return val;

            path_resolving::resolve(tes_context::instance(), obj, path, [&](item* itmPtr) {
                if (itmPtr) {
                    val = itmPtr->readAs<T>();
                }
            });

            return val;
        }
        REGISTERF(resolveGetter<Float32>, "solveFlt", "* path default=0.0", "attempts to get value at given path.\nJValue.solveInt(container, \".player.mood\") will return player's mood");
        REGISTERF(resolveGetter<SInt32>, "solveInt", "* path default=0", nullptr);
        REGISTERF(resolveGetter<skse::string_ref>, "solveStr", "* path default=\"\"", nullptr);
        REGISTERF(resolveGetter<Handle>, "solveObj", "* path default=0", nullptr);
        REGISTERF(resolveGetter<TESForm*>, "solveForm", "* path default=None", nullptr);

        template<class T>
        static bool solveSetter(object_base* obj, const char* path, T value, bool createMissingKeys = false) {
            if (!obj || !path)
                return false;

            bool succeed = ca::assign(*obj, path, value, createMissingKeys ? ca::creative : ca::constant);
/*
            path_resolving::resolve(tes_context::instance(), obj, path, [&](item* itmPtr) {
                if (itmPtr) {
                    *itmPtr = item((T)value);
                    succeed = true;
                }
            },
                createMissingKeys);
*/

            return succeed;
        }
        REGISTERF(solveSetter<Float32>, "solveFltSetter", "* path value createMissingKeys=false",
            "Attempts to assign value. Returns false if no such path\n"
            "With 'createMissingKeys=true' it creates any missing path element: solveIntSetter(map, \".keyA.keyB\", 10, true) on empty JMap creates {keyA: {keyB: 10}} structure"
            );
        REGISTERF(solveSetter<SInt32>, "solveIntSetter", "* path value createMissingKeys=false", nullptr);
        REGISTERF(solveSetter<const char*>, "solveStrSetter", "* path value createMissingKeys=false", nullptr);
        REGISTERF(solveSetter<ref>, "solveObjSetter", "* path value createMissingKeys=false", nullptr);
        REGISTERF(solveSetter<TESForm*>, "solveFormSetter", "* path value createMissingKeys=false", nullptr);

        template<class T>
        static T evalLua(ref obj, const char* luaCode, T def = T(0)) {
            auto result = lua::eval_lua_function(tes_context::instance(), obj, luaCode);
            return result ? result->readAs<T>() : def;
        }
        REGISTERF(evalLua<Float32>, "evalLuaFlt", "* luaCode default=0.0", "Evaluates piece of lua code. Lua support is experimental");
        REGISTERF(evalLua<SInt32>, "evalLuaInt", "* luaCode default=0", nullptr);
        REGISTERF(evalLua<skse::string_ref>, "evalLuaStr", "* luaCode default=\"\"", nullptr);
        REGISTERF(evalLua<Handle>, "evalLuaObj", "* luaCode default=0", nullptr);
        REGISTERF(evalLua<TESForm*>, "evalLuaForm", "* luaCode default=None", nullptr);

    };

    TES_META_INFO(tes_object);
}
