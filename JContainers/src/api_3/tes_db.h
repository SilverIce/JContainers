namespace tes_api_3 {

    using namespace collections;

    class tes_db : public class_meta<tes_db> {
    public:

        REGISTER_TES_NAME("JDB");

        void additionalSetup() {
            metaInfo.comment =
"Global entry point to store mod information. Main intent - replace global variables\n\
Manages keys and values associations as JMap";
        }

        template<class T>
        static T solveGetter(const char* path, T t = T(0)) {
            return tes_object::resolveGetter<T>(tes_context::instance().database(), path, t); 
        }
        REGISTERF(solveGetter<Float32>, "solveFlt", "path default=0.0",
"attempts to get value associated with path.\n\
for ex. following information associated with 'frosfall' key:\n\
\n\
\"frostfall\" : {\n\
    \"exposureRate\" : 0.5,\n\
    \"arrayC\" : [\"stringValue\", 1.5, 10, 1.14]\n\
}\n\
\n\
then JDB.solveFlt(\".frostfall.exposureRate\") will return 0.5 and\n\
JDB.solveObj(\".frostfall.arrayC\") will return array containing [\"stringValue\", 1.5, 10, 1.14] values");

        REGISTERF(solveGetter<SInt32>, "solveInt", "path default=0", nullptr);
        REGISTERF(solveGetter<const char*>, "solveStr", "path default=\"\"", nullptr);
        REGISTERF(solveGetter<Handle>, "solveObj", "path default=0", nullptr);
        REGISTERF(solveGetter<TESForm*>, "solveForm", "path default=None", nullptr);

        template<class T>
        static bool solveSetter(const char* path, T value, bool createMissingKeys = false) { 
            return tes_object::solveSetter(tes_context::instance().database(), path, value, createMissingKeys);
        }
        REGISTERF(solveSetter<Float32>, "solveFltSetter", "path value createMissingKeys=false",
            "Attempts to assign value. Returns false if no such path\n"
            "With 'createMissingKeys=true' it creates any missing path elements: JDB.solveIntSetter(\".frostfall.keyB\", 10, true) creates {frostfall: {keyB: 10}} structure");
        REGISTERF(solveSetter<SInt32>, "solveIntSetter", "path value createMissingKeys=false", nullptr);
        REGISTERF(solveSetter<const char*>, "solveStrSetter", "path value createMissingKeys=false", nullptr);
        REGISTERF(solveSetter<object_stack_ref&>, "solveObjSetter", "path value createMissingKeys=false", nullptr);
        REGISTERF(solveSetter<TESForm*>, "solveFormSetter", "path value createMissingKeys=false", nullptr);


        static void setObj(const char *path, object_stack_ref& obj) {
            map *dbMap = tes_context::instance().database();

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
"Associates(and replaces previous association) container object with a string key.\n\
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
            auto& context = tes_context::instance();
            auto objNew = json_deserializer::object_from_file(context, path);
            context.setDataBase(objNew);
        }
        REGISTERF2(readFromFile, "path",
"reads information from a file at given path and fills storage with it's JSON content\n\
NOTE: it will replace all existing JDB contents!");

    };

    TES_META_INFO(tes_db);
}