namespace collections {
    class tes_array : public tes_binding::class_meta_mixin< tes_array > {
    public:

        REGISTER_TES_NAME("JArray");

        static void additionalSetup() {
            //metaInfo().extendsClass = "JValue";
            metaInfo().comment = "Resizeable, unlimited size array (Skyrim size limit is 128) that may contain any value (value is float, integer, string or another container) in one time.\n"
                "Inherits all JValue functions";
        }

        static const char * TesName() { return "JArray";}

        typedef array::Index Index;

        static array* find(HandleT handle) {
            return collection_registry::getObjectOfType<array>(handle);
        }

        REGISTERF(tes_object::object<array>, "object", "", kCommentObject);

        static object_base* objectWithSize(UInt32 size) {
            auto obj = array::objectWithInitializer([&](array *me) {
                me->_array.resize(size);
            });

            return obj;
        }
        REGISTERF2(objectWithSize, "size", "creates array of given size, filled with empty items");

        template<class T>
        static object_base* fromArray(VMArray<T> arr) {
            auto obj = array::objectWithInitializer([&](array *me) {
                for (UInt32 i = 0; i < arr.Length(); ++i) {
                    T val;
                    arr.Get(&val, i);
                    me->_array.push_back(Item(val));
                }
            });

            return obj;
        }
        REGISTERF(fromArray<SInt32>, "objectWithInts", "values", "creates new array that contains given values");
        REGISTERF(fromArray<BSFixedString>, "objectWithStrings",  "values", NULL);
        REGISTERF(fromArray<Float32>, "objectWithFloats",  "values", NULL);
        REGISTERF(fromArray<bool>, "objectWithBooleans",  "values", NULL);

        template<class T>
        static T itemAtIndex(array *obj, Index index) {
            if (!obj) {
                return T(0);
            }

            mutex_lock g(obj->_mutex);
            return (index >= 0 && index < obj->_array.size()) ? obj->_array[index].readAs<T>() : T(0);
        }
        REGISTERF(itemAtIndex<SInt32>, "getInt", "* index", "returns item at index. getObj function returns container");
        REGISTERF(itemAtIndex<Float32>, "getFlt", "* index", "");
        REGISTERF(itemAtIndex<const char *>, "getStr", "* index", "");
        REGISTERF(itemAtIndex<Handle>, "getObj", "* index", "");
        REGISTERF(itemAtIndex<TESForm*>, "getForm", "* index", "");

        template<class T>
        static SInt32 findVal(array *obj, T value) {
            if (!obj) {
                return -1;
            }

            mutex_lock g(obj->_mutex);

            auto itr = std::find_if(obj->_array.begin(), obj->_array.end(), [=](const Item& item) {
                return item.isEqual(value);
            });

            return itr != obj->_array.end() ? (itr - obj->_array.begin()) : -1;
        }
        REGISTERF(findVal<SInt32>, "findInt", "* value", "returns index of the first found value/container that equals to given value/container.\n\
                                                         if found nothing returns -1.");
        REGISTERF(findVal<Float32>, "findFlt", "* value", "");
        REGISTERF(findVal<const char *>, "findStr", "* value", "");
        REGISTERF(findVal<object_base*>, "findObj", "* container", "");
        REGISTERF(findVal<TESForm*>, "findForm", "* value", "");

        template<class T>
        static void replaceItemAtIndex(array *obj, Index index, T item) {
            if (!obj) {
                return;
            }

            mutex_lock g(obj->_mutex);
            if (index >= 0 && index < obj->_array.size()) {
                obj->_array[index] = Item(item);
            }
        }
        REGISTERF(replaceItemAtIndex<SInt32>, "setInt", "* index value", "replaces existing value at index with new value. setObj sets container");
        REGISTERF(replaceItemAtIndex<Float32>, "setFlt", "* index value", "");
        REGISTERF(replaceItemAtIndex<const char *>, "setStr", "* index value", "");
        REGISTERF(replaceItemAtIndex<object_base*>, "setObj", "* index container", "");
        REGISTERF(replaceItemAtIndex<TESForm*>, "setForm", "* index value", "");

        template<class T>
        static void add(array *obj, T item) {
            if (obj) {
                mutex_lock g(obj->_mutex);
                obj->_array.push_back(Item(item));
            }
        }
        REGISTERF(add<SInt32>, "addInt", "* value", "appends value/container to the end of array");
        REGISTERF(add<Float32>, "addFlt", "* value", "");
        REGISTERF(add<const char *>, "addStr", "* value", "");
        REGISTERF(add<object_base*>, "addObj", "* container", "");
        REGISTERF(add<TESForm*>, "addForm", "* value", "");

        static Index count(array *obj) {
            if (obj) {
                mutex_lock g(obj->_mutex);
                return  obj->_array.size();
            }
            return 0;
        }
        REGISTERF2(count, "*", "returns number of items in array");

        static void clear(array *obj) {
            if (obj) {
                mutex_lock g(obj->_mutex);
                obj->_array.clear();
            }
        }
        REGISTERF2(clear, "*", "removes all items from array");

        static void eraseIndex(array *obj, SInt32 index) {
            if (obj) {
                mutex_lock g(obj->_mutex);
                if (index >= 0 && index < obj->_array.size()) {
                    obj->_array.erase(obj->_array.begin() + index);
                }
            }
        }
        REGISTERF2(eraseIndex, "* index", "erases item at index");

        static bool registerFuncs(VMClassRegistry* registry) {
            bind(registry);
            return true;
        }
    };

}
