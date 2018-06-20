namespace tes_api_3 {

/// Redefine in each logging module
#undef  JC_LOG_API_SOURCE
#define JC_LOG_API_SOURCE "JDB"

    using namespace collections;

    class tes_db : public class_meta<tes_db> {
    public:

        REGISTER_TES_NAME("JDB");

        void additionalSetup() {
            metaInfo.comment =
"Global entry point to store mod information. Main intent - replace global variables\n\
Manages keys and values associations (like JMap)";
        }

        template<class T>
        static T solveGetter(tes_context& ctx, const char* path, T t= default_value<T>())
        {
            JC_LOG_API ("%s, ...", path ? path : "");
            return tes_object::resolveGetter<T>(ctx, &ctx.root(), path, t);
        }
        REGISTERF(solveGetter<Float32>, "solveFlt", "path default=0.0",
"Attempts to retrieve the value associated with the @path.\n\
For ex. the following information associated with 'frosfall' key:\n\
\n\
\"frostfall\" : {\n\
    \"exposureRate\" : 0.5,\n\
    \"arrayC\" : [\"stringValue\", 1.5, 10, 1.14]\n\
}\n\
\n\
then JDB.solveFlt(\".frostfall.exposureRate\") will return 0.5 and\n\
JDB.solveObj(\".frostfall.arrayC\") will return the array containing [\"stringValue\", 1.5, 10, 1.14] values");

        REGISTERF(solveGetter<SInt32>, "solveInt", "path default=0", nullptr);
        REGISTERF(solveGetter<skse::string_ref>, "solveStr", "path default=\"\"", nullptr);
        REGISTERF(solveGetter<object_base*>, "solveObj", "path default=0", nullptr);
        REGISTERF(solveGetter<form_ref>, "solveForm", "path default=None", nullptr);

        template<class T>
        static bool solveSetter(tes_context& ctx, const char* path, T value, bool createMissingKeys = false)
        {
            JC_LOG_API ("%s, ..., %d", path ? path : "", int (createMissingKeys));
            return tes_object::solveSetter(ctx, &ctx.root(), path, value, createMissingKeys);
        }
        REGISTERF(solveSetter<Float32>, "solveFltSetter", "path value createMissingKeys=false",
            "Attempts to assign the @value. Returns false if no such path.\n"
            "If 'createMissingKeys=true' it creates any missing path elements: JDB.solveIntSetter(\".frostfall.keyB\", 10, true) creates {frostfall: {keyB: 10}} structure");
        REGISTERF(solveSetter<SInt32>, "solveIntSetter", "path value createMissingKeys=false", nullptr);
        REGISTERF(solveSetter<const char*>, "solveStrSetter", "path value createMissingKeys=false", nullptr);
        REGISTERF(solveSetter<object_base*>, "solveObjSetter", "path value createMissingKeys=false", nullptr);
        REGISTERF(solveSetter<form_ref>, "solveFormSetter", "path value createMissingKeys=false", nullptr);


        static void setObj(tes_context& ctx, const char *path, object_stack_ref& obj)
        {
            JC_LOG_API ("%s, ...", path ? path : "");

            map& dbMap = ctx.root();

            if (obj) {
                tes_map::setItem(ctx, &dbMap, path, obj);
            } else {
                tes_map::removeKey(ctx, &dbMap, path);
            }
        }
        REGISTERF(setObj, "setObj", "key object",
"Associates(and replaces previous association) container object with a string key.\n\
destroys association if object is zero\n\
for ex. JDB.setObj(\"frostfall\", frostFallInformation) will associate 'frostall' key and frostFallInformation so you can access it later"
);

        static bool hasPath(tes_context& ctx, const char* path) {
            JC_LOG_API ("%s", path ? path : "");
            return tes_object::hasPath(ctx, &ctx.root(), path);
        }
        REGISTERF2(hasPath, "path", "Returns true, if JDB capable resolve given @path, i.e. if it able to execute solve* or solver*Setter functions successfully");

        static object_base* allKeys(tes_context& ctx) {
            JC_LOG_API ("");
            return tes_map::allKeys(ctx, &ctx.root());
        }
        REGISTERF2(allKeys, "*", "returns new array containing all JDB keys");

        static object_base* allValues(tes_context& ctx) {
            JC_LOG_API ("");
            return tes_map::allValues(ctx, &ctx.root());
        }
        REGISTERF2(allValues, "*", "returns new array containing all containers associated with JDB");

        static void writeToFile(tes_context& ctx, const char * path) {
            JC_LOG_API ("%s", path ? path : "");
            tes_object::writeToFile(ctx, &ctx.root(), path);
        }
        REGISTERF2(writeToFile, "path", "writes storage data into JSON file at given path");

        static object_base* root(tes_context& ctx) {
            JC_LOG_API ("");
            return &ctx.root();
        }
        REGISTERF2(root, "", "Returns underlying JDB's container - an instance of JMap.\nThe object being owned (retained) internally, so you don't have to (but can) retain or release it.")
    };

    TES_META_INFO(tes_db);
}
