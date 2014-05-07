namespace collections {

    class tes_form_db : public tes_binding::class_meta_mixin_t<tes_form_db> {
    public:

        REGISTER_TES_NAME("JFormDB");

        void additionalSetup() {
            metaInfo.comment = "Manages form related information (entry).\n";
        }

        class subpath_extractor {
            enum { bytesCount = 0xff };

            const char *_rest;
            char _storageName[bytesCount];
        public:

            explicit subpath_extractor(const char * const path) {
                _storageName[0] = '\0';
                _rest = nullptr;

                if (path && *path == '.' && *(path + 1)) {
                    const char *pathPtr = path;
                    ++pathPtr;

                    int itr = 0;
                    while (*pathPtr && *pathPtr != '.' && *pathPtr != '[' && *pathPtr != ']' && itr < bytesCount) {
                        _storageName[itr++] = *pathPtr++;
                    }

                    if (itr > 0 && itr < (bytesCount - 1)) { // something was written into _storageName
                        _storageName[itr] = '\0';
                        _rest = pathPtr;
                    }
                }
            }

            const char *rest() const {
                return _rest;
            }

            const char *storageName() const {
                return _rest ? _storageName : nullptr;
            }
        };

        static form_map *makeFormStorage(const char *storageName) {
            if (!storageName) {
                return nullptr;
            }

            auto db = tes_context::instance().database();
            form_map *fmap = tes_map::getItem<object_base*>(db, storageName)->as<form_map>();

            if (!fmap) {
                fmap = tes_object::object<form_map>();
                tes_db::setObj(storageName, fmap);
            }

            return fmap;
        }

        static void setEntry(const char *storageName, TESForm *formKey, object_base *entry) {
            if (!storageName || !formKey) {
                return;
            }

            if (entry) {
                auto fmap = makeFormStorage(storageName);
                tes_form_map::setItem(fmap, formKey, entry);
            } else {
                auto db = tes_context::instance().database();
                auto fmap = tes_map::getItem<object_base*>(db, storageName)->as<form_map>();
                tes_form_map::removeKey(fmap, formKey);
            }
        }
        REGISTERF2(setEntry, "storageName fKey entry", "associates given form key and entry (container). set entry to zero to destroy association");

        static map *makeMapEntry(const char *storageName, TESForm *form) {
            if (!form || !storageName) {
                return nullptr;
            }

            form_map *fmap = makeFormStorage(storageName);
            map *entry = tes_form_map::getItem<object_base*>(fmap, form)->as<map>();
            if (!entry) {
                entry = tes_object::object<map>();
                tes_form_map::setItem(fmap, form, entry);
            }

            return entry;
        }
        REGISTERF(makeMapEntry, "makeEntry", "storageName fKey", "returns (or creates new if not found) JMap entry for given storage and form");

        static object_base *findEntry(const char *storageName, TESForm *form) {
            auto db = tes_context::instance().database();
            form_map *fmap = tes_map::getItem<object_base*>(db, storageName)->as<form_map>();
            return tes_form_map::getItem<object_base*>(fmap, form);
        }
        REGISTERF2(findEntry, "storageName fKey", "search for entry for given storage and form");

        static map *findMapEntry(const char *storageName, TESForm *form) {
            return findEntry(storageName, form)->as<map>();
        }

        //////////////////////////////////////////////////////////////////////////

        template<class T>
        static T solveGetter(TESForm *form, const char* path) {
            subpath_extractor sub(path);
            return tes_object::resolveGetter<T>(findEntry(sub.storageName(), form), sub.rest()); 
        }
        REGISTERF(solveGetter<Float32>, "solveFlt", "fKey path", "attempts to get value associated with path.");
        REGISTERF(solveGetter<SInt32>, "solveInt", "fKey path", NULL);
        REGISTERF(solveGetter<const char*>, "solveStr", "fKey path", NULL);
        REGISTERF(solveGetter<object_base*>, "solveObj", "fKey path", NULL);
        REGISTERF(solveGetter<TESForm*>, "solveForm", "fKey path", NULL);

        template<class T>
        static bool solveSetter(TESForm *form, const char* path, T value) { 
            subpath_extractor sub(path);
            return tes_object::solveSetter(findEntry(sub.storageName(), form), sub.rest(), value);
        }
        REGISTERF(solveSetter<Float32>, "solveFltSetter", "fKey path value",
            "attempts to assign value. returns false if no such path");
        REGISTERF(solveSetter<SInt32>, "solveIntSetter", "fKey path value", NULL);
        REGISTERF(solveSetter<const char*>, "solveStrSetter", "fKey path value", NULL);
        REGISTERF(solveSetter<object_base*>, "solveObjSetter", "fKey path value", NULL);
        REGISTERF(solveSetter<TESForm*>, "solveFormSetter", "fKey path value", NULL);

        static bool hasPath(TESForm *form, const char* path) {
            subpath_extractor sub(path);
            return tes_object::hasPath(findMapEntry(sub.storageName(), form), sub.rest());
        }
        REGISTERF2(hasPath, "fKey path", "returns true, if capable resolve given path, e.g. it able to execute solve* or solver*Setter functions successfully");

        //////////////////////////////////////////////////////////////////////////

        static object_base* allKeys(TESForm *form, const char *path) {
            subpath_extractor sub(path);
            return tes_map::allKeys( findMapEntry(sub.storageName(), form) );
        }
        REGISTERF2(allKeys, "fKey key",
            "JMap-like interface functions:\n"
            "\n"
            "returns new array containing all keys");

        static object_base* allValues(TESForm *form, const char *path) {
            subpath_extractor sub(path);
            return tes_map::allValues( findMapEntry(sub.storageName(), form) );
        }
        REGISTERF2(allValues, "fKey key", "returns new array containing all values");

        template<class T>
        static T getItem(TESForm *form, const char* path) {
            subpath_extractor sub(path);
            return tes_map::getItem<T>( findMapEntry(sub.storageName(), form), sub.rest());
        }
        REGISTERF(getItem<SInt32>, "getInt", "fKey key", "returns value associated with key");
        REGISTERF(getItem<Float32>, "getFlt", "fKey key", "");
        REGISTERF(getItem<const char *>, "getStr", "fKey key", "");
        REGISTERF(getItem<object_base *>, "getObj", "fKey key", "");
        REGISTERF(getItem<TESForm*>, "getForm", "fKey key", "");

        template<class T>
        static void setItem(TESForm *form, const char* path, T item) {
            subpath_extractor sub(path);
            tes_map::setItem( makeMapEntry(sub.storageName(), form), sub.rest(), item);
        }
        REGISTERF(setItem<SInt32>, "setInt", "fKey key value", "creates key-value association. replaces existing value if any");
        REGISTERF(setItem<Float32>, "setFlt", "fKey key value", "");
        REGISTERF(setItem<const char *>, "setStr", "fKey key value", "");
        REGISTERF(setItem<object_base*>, "setObj", "fKey key container", "");
        REGISTERF(setItem<TESForm*>, "setForm", "fKey key value", "");
    };

    TES_META_INFO(tes_form_db);

    TEST(tes_form_db, subpath_extractor)
    {
        auto expectEq = [&](const char *path, const char *storageName, const char *rest) {
            auto extr = tes_form_db::subpath_extractor(path);
            EXPECT_TRUE((!storageName && !extr.storageName()) || strcmp(extr.storageName(), storageName) == 0 );
            EXPECT_TRUE((!rest && !extr.rest()) || strcmp(extr.rest(), rest) == 0 );
        };

        expectEq(".strg.key", "strg", ".key");
        expectEq(".strg[0]", "strg", "[0]");
        expectEq(".strg[0].key.key2", "strg", "[0].key.key2");

        expectEq("strg[0]", nullptr, nullptr);
        expectEq("[", nullptr, nullptr);
        expectEq(nullptr, nullptr, nullptr);
        expectEq("", nullptr, nullptr);
        expectEq("...", nullptr, nullptr);
    }
}
