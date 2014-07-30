namespace collections {

    inline const char* tes_hash(const char* in) {
        return in;
    }

    inline FormId tes_hash(TESForm * in) {
        return (FormId) (in ? in->formID : 0); 
    }

    template<class Key, class Cnt>
    class tes_map_t : public reflection::class_meta_mixin_t< tes_map_t<Key, Cnt> > {
    public:

        typedef typename Cnt::ref& ref;

        REGISTERF(tes_object::object<Cnt>, "object", "", kCommentObject);

        template<class T>
        static T getItem(Cnt *obj, Key key, T def = T(0)) {
            if (!obj || !key) {
                return def;
            }

            object_lock g(obj);
            auto item = obj->u_find(tes_hash(key));
            return item ? item->readAs<T>() : def;
        }
        template<class T>
        static T _getItem(ref obj, Key key, T def) {
            return getItem<T>(obj.get(), key);
        }
        REGISTERF(_getItem<SInt32>, "getInt", "object key default=0", "returns value associated with key");
        REGISTERF(_getItem<Float32>, "getFlt", "object key default=0.0", "");
        REGISTERF(_getItem<const char *>, "getStr", "object key default=\"\"", "");
        REGISTERF(_getItem<Handle>, "getObj", "object key default=0", "");
        REGISTERF(_getItem<TESForm*>, "getForm", "object key default=None", "");

        template<class T>
        static void setItem(Cnt *obj, Key key, T item) {
            if (!obj || !key) {
                return;
            }
            obj->setValueForKey(tes_hash(key), Item(item));
        }

        template<class T>
        static void _setItem(ref obj, Key key, T item) {
            setItem<T>(obj.get(), key, item);
        }
        REGISTERF(_setItem<SInt32>, "setInt", "* key value", "creates key-value association. replaces existing value if any");
        REGISTERF(_setItem<Float32>, "setFlt", "* key value", "");
        REGISTERF(_setItem<const char *>, "setStr", "* key value", "");
        REGISTERF(_setItem<object_stack_ref&>, "setObj", "* key container", "");
        REGISTERF(_setItem<TESForm*>, "setForm", "* key value", "");

        static bool hasKey(ref obj, Key key) {
            return valueType(obj, key) != 0;
        }
        REGISTERF2(hasKey, "* key", "returns true, if something associated with key");

        static SInt32 valueType(ref obj, Key key) {
            if (!obj || !key) {
                return 0;
            }

            object_lock g(obj);
            auto item = obj->u_find(tes_hash(key));
            return item ? item->which() : 0;
        }
        REGISTERF2(valueType, "* key", "returns type of the value associated with key.\n"VALUE_TYPE_COMMENT);

        static object_base* allKeys(Cnt* obj) {
            if (!obj) {
                return nullptr;
            }

            return array::objectWithInitializer([&](array *arr) {
                object_lock g(obj);

                arr->_array.reserve(obj->u_count());
                for each(auto& pair in obj->u_container()) {
                    arr->u_push(Item(pair.first));
                }
            },
                tes_context::instance());
        }

        static object_base* _allKeys(ref obj) {
            return allKeys(obj.get());
        }
        REGISTERF(_allKeys, "allKeys", "*", "returns new array containing all keys");

        static object_base* allValues(Cnt *obj) {
            if (!obj) {
                return nullptr;
            }

            return array::objectWithInitializer([&](array *arr) {
                object_lock g(obj);

                arr->_array.reserve(obj->u_count());
                for each(auto& pair in obj->u_container()) {
                    arr->_array.push_back(pair.second);
                }
            },
                tes_context::instance());
        }


        static object_base* _allValues(ref obj) {
            return allValues(obj.get());
        }
        REGISTERF(_allValues, "allValues", "*", "returns new array containing all values");

        static bool removeKey(Cnt *obj, Key key) {
            if (!obj || !key) {
                return 0;
            }

            object_lock g(obj);
            return obj->u_erase(tes_hash(key));
        }

        static bool _removeKey(ref obj, Key key) {
            return removeKey(obj.get(), key);
        }
        REGISTERF(_removeKey, "removeKey", "* key", "destroys key-value association");

        static SInt32 count(ref obj) {
            if (!obj) {
                return 0;
            }

            return obj->s_count();
        }
        REGISTERF2(count, "*", "returns count of items/associations");

        static void clear(ref obj) {
            if (!obj) {
                return;
            }

            obj->s_clear();
        }
        REGISTERF2(clear, "*", "removes all items from container");

        static void addPairs(ref obj, const ref source, bool overrideDuplicates) {
            if (!obj || !source || source == obj) {
                return;
            }

            object_lock g(obj);
            object_lock c(source);

            if (overrideDuplicates) {
                for (const auto& pair : source->u_container()) {
                    obj->u_container()[pair.first] = pair.second;
                }
            }
            else {
                obj->u_container().insert(source->u_container().begin(), source->u_container().end());
            }
        }
        REGISTERF2(addPairs, "* source overrideDuplicates", "inserts key-value pairs from the source map");

        void additionalSetup();
    };

    typedef tes_map_t<const char*, map > tes_map;
    typedef tes_map_t<TESForm *, form_map> tes_form_map;

    void tes_map::additionalSetup() {
        metaInfo.className = "JMap";
        metaInfo.comment = "Associative key-value container.\n"
            "Inherits JValue functionality";
    }

    void tes_form_map::additionalSetup() {
        metaInfo.className = "JFormMap";
        metaInfo.comment = "Associative key-value container.\n"
            "Inherits JValue functionality";
    }

    TES_META_INFO(tes_map);
    TES_META_INFO(tes_form_map);
}
