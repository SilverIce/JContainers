namespace collections {

    const char *kCommentObject = "creates new container object. returns container identifier (integer number).\n"
        "identifier is the thing you will have to pass to the most of container's functions as a first argument";

#define ARGS(...)   #__VA_ARGS__

    class tes_object : public reflection::class_meta_mixin_t< tes_object > {
    public:

        typedef object_stack_ref& ref;

        REGISTER_TES_NAME("JValue");

        void additionalSetup() {
            metaInfo.comment = "Each container (JArray, JMap & JFormMap) inherits JValue functionality";
        }

        static object_base* retain(ref obj, const char* tag = nullptr) {
            if (obj) {
                obj->tes_retain();

                if (tag) {
                    obj->set_tag(tag);
                }

                return obj.get();
            }

            return nullptr;
        }
        REGISTERF2(retain, "* tag=None",
"Retains and returns the object. Purpose - extend object lifetime.\n\
Newly created object if not retained or not referenced/contained by another container directly or indirectly gets destoyed after ~10 seconds due to absence of owners.\n\
Retain increases amount of owners object have by 1. The retainer is responsible for releasing object later.\n\
Object have extended lifetime if JDB or JFormDB or any other container references/owns/contains object directly or indirectly.\n\
It's recommended to set a tag (any unique string will fit - mod name for ex.) - later you'll be able to release all objects with selected tag even if identifier has been lost"
            );

        template<class T>
        static T* object() {
            return T::object(tes_context::instance());
        }

        static object_base* release(ref obj) {
            if (obj) {
                obj->tes_release();
            }

            return nullptr;
        }
        REGISTERF2(release, "*", "releases the object and returns zero, so you can release and nullify with one line of code: object = JValue.release(object)");

        static object_base* releaseAndRetain(ref previousObject, ref newObject, const char* tag = nullptr) {
            if (previousObject != newObject) {
                if (previousObject) {
                    previousObject->tes_release();
                }

                retain(newObject, tag);
            }

            return newObject.get();
        }
        REGISTERF2(releaseAndRetain, "previousObject newObject tag=None",
"Just a union of retain-release calls. Releases previousObject, retains and returns newObject.\n\
It's recommended to set tag (any unique string will fit - mod name for ex.) - later you'll be able to release all objects with selected tag even if identifier was lost.");

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
"For maintenance - releases lost (and not lost) objects with given tag.\n"
"Complements all retain calls objects with given tag received with release calls.\n"
"See 'object lifetime management' section for more information");

        static bool isArray(ref obj) {
            return obj->as<array>() != nullptr;
        }
        REGISTERF2(isArray, "*", "returns true if object is map, array or formmap container");

        static bool isMap(ref obj) {
            return obj->as<map>() != nullptr;
        }
        REGISTERF2(isMap, "*", nullptr);

        static bool isFormMap(ref obj) {
            return obj->as<form_map>() != nullptr;
        }
        REGISTERF2(isFormMap, "*", nullptr);

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
        REGISTERF2(readFromFile, "filePath", "creates and returns new container object containing the contents of JSON file");

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

            map *files = map::object(tes_context::instance()); 

            for ( filesystem::directory_iterator itr( root ); itr != end_itr; ++itr ) {

                if ( filesystem::is_regular_file( *itr ) &&
                     (!*extension || itr->path().extension().generic_string().compare(extension) == 0) ) {
                    auto asniString = itr->path().generic_string();
                    auto jsonObject = tes_object::readFromFile(asniString.c_str());

                    if (jsonObject) {
                        files->setValueForKey(itr->path().filename().generic_string(), Item(jsonObject));
                    }  
                }
            }

            return files;
        }
        REGISTERF2(readFromDirectory, "directoryPath extension=\"\"",
            "parses JSON files in directory (non recursive) and returns JMap containing {filename, container-object} pairs.\n"
            "note: by default it does not filters files by extension and will try to parse everything");

        static object_base* objectFromPrototype(const char *prototype) {
            auto obj = json_deserializer::object_from_json_data( tes_context::instance(), prototype);
            return obj;
        }
        REGISTERF2(objectFromPrototype, "prototype", "creates new container object using given JSON string-prototype");

        static void writeToFile(object_base *obj, const char * path) {
            if (!path || !obj) {
                return;
            }

            auto json = json_serializer::create_json_value(*obj);
            if (json) {
                json_dump_file(json.get(), path, JSON_INDENT(2));
            }
        }
        static void _writeToFile(ref obj, const char * path) {
            writeToFile(obj.get(), path);
        }
        REGISTERF(_writeToFile, "writeToFile", "* filePath", "writes object into JSON file");

        static bool hasPath(object_base* obj, const char *path) {
            if (!obj || !path)
                return false;

            bool succeed = false;
            path_resolving::resolvePath(obj, path, [&](Item* itmPtr) {
                succeed = (itmPtr != nullptr);
            });

            return succeed;
        }

        static bool _hasPath(ref obj, const char *path) {
            return hasPath(obj.get(), path);
        }
        REGISTERF(_hasPath, "hasPath", "* path",
"returns true, if container capable resolve given path.\n\
for ex. JValue.hasPath(container, \".player.health\") will check if given container has 'player' which has 'health' information"
                                      );

        template<class T>
        static T resolveGetter(object_base *obj, const char* path) {
            if (!obj || !path)
                return 0;

            T val((T)0);
            path_resolving::resolvePath(obj, path, [&](Item* itmPtr) {
                if (itmPtr) {
                    val = itmPtr->readAs<T>();
                }
            });

            return val;
        }
        template<class T>
        static T _resolveGetter(ref obj, const char* path) {
            return resolveGetter<T>(obj.get(), path);
        }
        REGISTERF(_resolveGetter<Float32>, "solveFlt", "* path", "attempts to get value at given path.\nJValue.solveInt(container, \".player.mood\") will return player's mood");
        REGISTERF(_resolveGetter<SInt32>, "solveInt", "* path", nullptr);
        REGISTERF(_resolveGetter<const char*>, "solveStr", "* path", nullptr);
        REGISTERF(_resolveGetter<object_base*>, "solveObj", "* path", nullptr);
        REGISTERF(_resolveGetter<TESForm*>, "solveForm", "* path", nullptr);

        template<class T>
        static bool solveSetter(object_base* obj, const char* path, T value, bool createMissingKeys = false) {
            if (!obj || !path)
                return false;

            bool succeed = false;
            path_resolving::resolvePath(obj, path, [&](Item* itmPtr) {
                if (itmPtr) {
                    *itmPtr = Item((T)value);
                    succeed = true;
                }
            },
                createMissingKeys);

            return succeed;
        }

        template<class T>
        static bool _solveSetter(ref obj, const char* path, T value, bool createMissingKeys = false) {
            return solveSetter<T>(obj.get(), path, value, createMissingKeys);
        }
        REGISTERF(_solveSetter<Float32>, "solveFltSetter", "* path value createMissingKeys=false",
            "Attempts to assign value. Returns false if no such path\n"
            "With 'createMissingKeys=true' it creates any missing path element: solveIntSetter(map, \".keyA.keyB\", 10, true) on empty JMap creates {keyA: {keyB: 10}} structure"
            );
        REGISTERF(_solveSetter<SInt32>, "solveIntSetter", "* path value createMissingKeys=false", nullptr);
        REGISTERF(_solveSetter<const char*>, "solveStrSetter", "* path value createMissingKeys=false", nullptr);
        REGISTERF(_solveSetter<ref>, "solveObjSetter", "* path value createMissingKeys=false", nullptr);
        REGISTERF(_solveSetter<TESForm*>, "solveFormSetter", "* path value createMissingKeys=false", nullptr);

    };

    TES_META_INFO(tes_object);

}
