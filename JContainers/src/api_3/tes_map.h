namespace tes_api_3 {

    using namespace collections;

    inline const char* tes_hash(const char* in) {
        return in;
    }

    inline FormId tes_hash(TESForm * in) {
        return (FormId) (in ? in->formID : 0); 
    }

    template<class Key, class Cnt>
    class tes_map_t : public class_meta< tes_map_t<Key, Cnt> > {
    public:

        typedef typename Cnt* ref;

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
        REGISTERF(getItem<SInt32>, "getInt", "object key default=0", "returns value associated with key");
        REGISTERF(getItem<Float32>, "getFlt", "object key default=0.0", "");
        REGISTERF(getItem<const char *>, "getStr", "object key default=\"\"", "");
        REGISTERF(getItem<object_base*>, "getObj", "object key default=0", "");
        REGISTERF(getItem<TESForm*>, "getForm", "object key default=None", "");

        template<class T>
        static void setItem(Cnt *obj, Key key, T item) {
            if (!obj || !key) {
                return;
            }
            obj->setValueForKey(tes_hash(key), Item(item));
        }
        REGISTERF(setItem<SInt32>, "setInt", "* key value", "creates key-value association. replaces existing value if any");
        REGISTERF(setItem<Float32>, "setFlt", "* key value", "");
        REGISTERF(setItem<const char *>, "setStr", "* key value", "");
        REGISTERF(setItem<object_base*>, "setObj", "* key container", "");
        REGISTERF(setItem<TESForm*>, "setForm", "* key value", "");

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
            return item ? item->type() : item_type::no_item;
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
        REGISTERF(allKeys, "allKeys", "*", "returns new array containing all keys");

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
        REGISTERF(allValues, "allValues", "*", "returns new array containing all values");

        static bool removeKey(Cnt *obj, Key key) {
            if (!obj || !key) {
                return 0;
            }

            object_lock g(obj);
            return obj->u_erase(tes_hash(key));
        }
        REGISTERF(removeKey, "removeKey", "* key", "destroys key-value association");

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
        metaInfo._className = "JMap";
        metaInfo.comment = "Associative key-value container.\n"
            "Inherits JValue functionality";
    }

    void tes_form_map::additionalSetup() {
        metaInfo._className = "JFormMap";
        metaInfo.comment = "Associative key-value container.\n"
            "Inherits JValue functionality";
    }

    TES_META_INFO(tes_map);
    TES_META_INFO(tes_form_map);
}
