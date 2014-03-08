namespace collections {
    class tes_db : public tes_binding::class_meta_mixin_t<tes_db> {
    public:

        REGISTER_TES_NAME("JDB");

        void additionalSetup() {
            metaInfo.comment =
"Global entry point to store mod information. Main intent - replace global variables\n\
Manages keys and values associations as JMap";
        }

        template<class T>
        static T solveGetter(const char* path) {
            return tes_object::resolveGetter<T>(tes_context::instance().database(), path); 
        }
        REGISTERF(solveGetter<Float32>, "solveFlt", "path",
"attempts to get value assiciated with path.\n\
for ex. following information associated with 'frosfall' key:\n\
\n\
\"frostfall\" = {\n\
    \"exposureRate\" : 0.5,\n\
    \"arrayC\" : [\"stringValue\", 1.5, 10, 1.14]\n\
}\n\
\n\
then JDB.solveFlt(\".frostfall.exposureRate\") will return 0.5 and\n\
JDB.solveObj(\".frostfall.arrayC\") will return array containing [\"stringValue\", 1.5, 10, 1.14] values");

        REGISTERF(solveGetter<SInt32>, "solveInt", "path", NULL);
        REGISTERF(solveGetter<const char*>, "solveStr", "path", NULL);
        REGISTERF(solveGetter<object_base*>, "solveObj", "path", NULL);
        REGISTERF(solveGetter<TESForm*>, "solveForm", "path", NULL);

        static void setObj(const char *path, object_base *obj) {
            object_base *db = tes_context::instance().database();
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
        REGISTERF(setObj, "setObj", "key object",
"Associates(and replaces previous association) container object (array or map) with a string key.\n\
destroys association if object is zero\n\
for ex. JDB.setObj(\"frostfall\", frostFallInformation) will associate 'frostall' key and frostFallInformation so you can access it later"
);

        static bool hasPath(const char* path) {
            return tes_object::hasPath(tes_context::instance().database(), path);
        }
        REGISTERF2(hasPath, "path", "returns true, if DB capable resolve given path, e.g. it able to execute solve* or solver*Setter functions successfully");

        static object_base* allKeys() {
            return tes_map::allKeys( tes_context::instance().database()->as<map>() );
        }
        REGISTERF2(allKeys, "*", "returns new array containing all JDB keys");

        static object_base* allValues() {
            return tes_map::allValues( tes_context::instance().database()->as<map>() );
        }
        REGISTERF2(allValues, "*", "returns new array containing all containers associated with JDB");

        static void writeToFile(const char * path) {
            tes_object::writeToFile( tes_context::instance().database(), path);
        }
        REGISTERF2(writeToFile, "path", "writes storage data into JSON file at given path");

        static void readFromFile(const char *path) {
            auto objNew = json_handling::readJSONFile(path);
            tes_context::instance().setDataBase(objNew);
        }
        REGISTERF2(readFromFile, "path",
"reads information from a file at given path and fills storage with it's JSON content\n\
NOTE: it will replace all existing JDB contents!");

        template<class T>
        static bool solveSetter(const char* path, T value) { 
            return tes_object::solveSetter(tes_context::instance().database(), path, value);
        }
        REGISTERF(solveSetter<Float32>, "solveFltSetter", "path value",
"attempts to assign value. returns false if no such path\n\
for ex. JDB.solveFltSetter(\".frostfall.exposureRate\", 1.0) assigns 1.0 to \".frostfall.exposureRate\" path");
        REGISTERF(solveSetter<SInt32>, "solveIntSetter", "path value", NULL);
        REGISTERF(solveSetter<const char*>, "solveStrSetter", "path value", NULL);
        REGISTERF(solveSetter<object_base*>, "solveObjSetter", "path value", NULL);
        REGISTERF(solveSetter<TESForm*>, "solveFormSetter", "path value", NULL);

    };

    TES_META_INFO(tes_db);
}