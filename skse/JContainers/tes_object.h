namespace collections {

    const char *kCommentObject = "creates new container object. returns container identifier (integral number).\n"
        "identifier is the thing you will have to pass to the most of container's functions as first argument";

#define ARGS(...)   #__VA_ARGS__

    class tes_object : public tes_binding::class_meta_mixin< tes_object > {
    public:

        REGISTER_TES_NAME("JValue");

        static void additionalSetup() {
            metaInfo().comment = "Each container (JArray, JMap & JFormMap) inherits JValue functionality";
        }

        static object_base* retain(object_base *obj) {
            if (obj) {
                obj->tes_retain();
                return obj;
            }

            return nullptr;
        }
        REGISTERF2(retain, "*",
"Retains and returns the object.\n\
All containers that were created with object* or objectWith* methods are automatically destroyed after some amount of time (~10 seconds)\n\
To keep object alive you must retain it once and you have to __release__ it when you do not need it anymore (also to not pollute save file).\n\
An alternative to retain-release is store object in JDB container"
            );

        static HandleT autorelease(HandleT handle) {
            autorelease_queue::instance().push(handle);
            return handle;
        }

        template<class T>
        static object_base* object() {
            return T::object();
        }

        static void release(object_base *obj) {
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

        static bool empty(object_base *obj) {
            return count(obj) == 0;
        }
        REGISTERF2(empty, "*", "returns true, if container is empty");

        static SInt32 count(object_base *obj) {
            return obj ? obj->s_count() : 0;
        }
        REGISTERF2(count, "*", "returns the number of items in container");

        static void clear(object_base *obj) {
            if (obj) {
                obj->s_clear();
            }
        }
        REGISTERF2(clear, "*", "removes all items from container");

        static object_base* readFromFile(const char *path) {
            if (path == nullptr)
                return 0;

            auto obj = json_handling::readJSONFile(path);
            return  obj;
        }
        REGISTERF2(readFromFile, "filePath", ARGS(creates and returns new container (JArray or JMap) containing the contents of JSON file));

        static object_base* objectFromPrototype(const char *prototype) {
            if (!prototype)
                return nullptr;

            auto obj = json_handling::readJSONData(prototype);
            return obj;
        }
        REGISTERF2(objectFromPrototype, "prototype", "creates new container object using given JSON string-prototype");

        static void writeToFile(object_base *obj, const char * path) {
            if (path == nullptr)  return;
            if (!obj)  return;

            std::unique_ptr<char, decltype(&free)> data(json_handling::createJSONData(*obj), &free);
            if (!data) return;

            auto file = make_unique_file(fopen(path, "w"));
            if (!file) {
                return;
            }

            fwrite(data.get(), 1, strlen(data.get()), file.get());
        }
        REGISTERF2(writeToFile, "* filePath", "writes object into JSON file");

        static bool hasPath(object_base *obj, const char *path) {
            if (!obj || !path)
                return false;

            bool succeed = false;
            json_handling::resolvePath(obj, path, [&](Item* itmPtr) {
                succeed = (itmPtr != nullptr);
            });

            return succeed;
        }
        REGISTERF2(hasPath, "* path",
"returns true, if container capable resolve given path.\n\
for ex. JValue.hasPath(container, \".player.health\") will check if given container has 'player' which has 'health' information"
                                      );

        template<class T>
        static T resolveGetter(object_base *obj, const char* path) {
            if (!obj || !path)
                return 0;

            T val((T)0);
            json_handling::resolvePath(obj, path, [&](Item* itmPtr) {
                if (itmPtr) {
                    val = itmPtr->readAs<T>();
                }
            });

            return val;
        }
        REGISTERF(resolveGetter<Float32>, "solveFlt", "* path", "attempts to get value at given path.\nJValue.solveInt(container, \".player.mood\") will return player's mood");
        REGISTERF(resolveGetter<SInt32>, "solveInt", "* path", NULL);
        REGISTERF(resolveGetter<const char*>, "solveStr", "* path", NULL);
        REGISTERF(resolveGetter<object_base*>, "solveObj", "* path", NULL);
        REGISTERF(resolveGetter<TESForm*>, "solveForm", "* path", NULL);

        template<class T>
        static bool solveSetter(object_base *obj, const char* path, T value) { 
            if (!obj || !path)
                return false;

            bool succeed = false;
            json_handling::resolvePath(obj, path, [&](Item* itmPtr) {
                if (itmPtr) {
                    *itmPtr = Item((T)value);
                    succeed = true;
                }
            });

            return succeed;
        }
        REGISTERF(solveSetter<Float32>, "solveFltSetter", "* path value", "attempts to set value.\nJValue.solveIntSetter(container, \".player.mood\", 12) will set player's mood to 12");
        REGISTERF(solveSetter<SInt32>, "solveIntSetter", "* path value", NULL);
        REGISTERF(solveSetter<const char*>, "solveStrSetter", "* path value", NULL);
        REGISTERF(solveSetter<object_base*>, "solveObjSetter", "* path value", NULL);
        REGISTERF(solveSetter<TESForm*>, "solveFormSetter", "* path value", NULL);


        static bool registerFuncs(VMClassRegistry* registry) {


            bind(registry);

            return true;
        }
    };

}
