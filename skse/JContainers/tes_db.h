namespace collections {
    class tes_db : public tes_binding::class_meta_mixin<tes_db> {
    public:

        REGISTER_TES_NAME("JDB");

        static void additionalSetup() {}

        template<class T>
        static T solveGetter(const char* path) {
            return tes_object::resolveGetter<T>(shared_state::instance().database(), path); 
        }
        REGISTERF(solveGetter<Float32>, "solveFlt", "path", NULL);
        REGISTERF(solveGetter<SInt32>, "solveInt", "path", NULL);
        REGISTERF(solveGetter<const char*>, "solveStr", "path", NULL);
        REGISTERF(solveGetter<object_base*>, "solveObj", "path", NULL);
        REGISTERF(solveGetter<TESForm*>, "solveForm", "path", NULL);

        static void setObj(const char *path, object_base *obj) {
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
        REGISTERF(setObj, "setObj", "key object", "");

        static bool hasPath(const char* path) {
            return tes_object::hasPath(shared_state::instance().database(), path);
        }
        REGISTERF2(hasPath, "path", "");

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

        static void readFromFile(const char *path) {
            auto objNew = json_handling::readJSONFile(path);
            shared_state::instance().setDataBase(objNew);
        }
        REGISTERF2(readFromFile, "path", "fills storage with JSON data");

        template<class T>
        static bool solveSetter(const char* path, T value) { 
            return tes_object::solveSetter(shared_state::instance().database(), path, value);
        }
        REGISTERF(solveSetter<Float32>, "solveFltSetter", "path value", NULL);
        REGISTERF(solveSetter<SInt32>, "solveIntSetter", "path value", NULL);
        REGISTERF(solveSetter<const char*>, "solveStrSetter", "path value", NULL);
        REGISTERF(solveSetter<object_base*>, "solveObjSetter", "path value", NULL);
        REGISTERF(solveSetter<TESForm*>, "solveFormSetter", "path value", NULL);

        static bool registerFuncs(VMClassRegistry* registry) {
            bind(registry);
            return true;
        }
    };
}