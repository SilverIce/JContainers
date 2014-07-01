namespace collections {

    inline const char* tes_hash(const char* in) {
        return in;
    }

    inline FormId tes_hash(TESForm * in) {
        return (FormId) (in ? in->formID : 0); 
    }

    template<class Key, class Cnt>
    class tes_map_t : public tes_binding::class_meta_mixin_t< tes_map_t<Key, Cnt> > {
    public:

        REGISTERF(tes_object::object<Cnt>, "object", "", kCommentObject);

        template<class T>
        static T getItem(Cnt *obj, Key key) {
            if (!obj || !key) {
                return T(0);
            }

            object_lock g(obj);
            auto item = obj->u_find(tes_hash(key));
            return item ? item->readAs<T>() : T(0);
        }
        REGISTERF(getItem<SInt32>, "getInt", "object key", "returns value associated with key");
        REGISTERF(getItem<Float32>, "getFlt", "object key", "");
        REGISTERF(getItem<const char *>, "getStr", "object key", "");
        REGISTERF(getItem<object_base *>, "getObj", "object key", "");
        REGISTERF(getItem<TESForm*>, "getForm", "object key", "");

        template<class T>
        static void setItem(Cnt *obj, Key key, T item) {
            if (!obj || !key) {
                return;
            }

            obj->setValueForKey( tes_hash(key), Item(item) );
        }
        REGISTERF(setItem<SInt32>, "setInt", "* key value", "creates key-value association. replaces existing value if any");
        REGISTERF(setItem<Float32>, "setFlt", "* key value", "");
        REGISTERF(setItem<const char *>, "setStr", "* key value", "");
        REGISTERF(setItem<object_base*>, "setObj", "* key container", "");
        REGISTERF(setItem<TESForm*>, "setForm", "* key value", "");

        static bool hasKey(Cnt *obj, Key key) {
            if (!obj || !key) {
                return 0;
            }

            object_lock g(obj);
            auto item = obj->u_find(tes_hash(key));
            return item != nullptr;
        }
        REGISTERF2(hasKey, "* key", "returns true, if something associated with key");

        static object_base* allKeys(Cnt *obj) {
            if (!obj) {
                return nullptr;
            }

            return array::objectWithInitializer([=](array *arr) {
                object_lock g(obj);

                arr->_array.reserve( obj->u_count() );
                for each(auto& pair in obj->u_container()) {
                    arr->u_push( Item(pair.first) );
                }
            },
                tes_context::instance());
        }
        REGISTERF2(allKeys, "*", "returns new array containing all keys");

        static object_base* allValues(Cnt *obj) {
            if (!obj) {
                return nullptr;
            }

            return array::objectWithInitializer([=](array *arr) {
                object_lock g(obj);

                arr->_array.reserve( obj->u_count() );
                for each(auto& pair in obj->u_container()) {
                    arr->_array.push_back( pair.second );
                }
            },
                tes_context::instance());
        }
        REGISTERF2(allValues, "*", "returns new array containing all values");

        static bool removeKey(Cnt *obj, Key key) {
            if (!obj || !key) {
                return 0;
            }

            object_lock g(obj);
            return obj->u_erase(tes_hash(key));
        }
        REGISTERF2(removeKey, "* key", "destroys key-value association");

        static SInt32 count(Cnt *obj) {
            if (!obj) {
                return 0;
            }

            return obj->s_count();
        }
        REGISTERF2(count, "*", "returns count of items/associations");

        static void clear(Cnt *obj) {
            if (!obj) {
                return;
            }

            obj->s_clear();
        }
        REGISTERF2(clear, "*", "removes all items from container");

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
