namespace collections {

    class tes_array : public tes_binding::class_meta_mixin< tes_array > {
    public:

        REGISTER_TES_NAME("JArray");

        static void additionalSetup() {
            metaInfo().comment = "Resizeable, unlimited size array (Skyrim size limit is 128) that may contain any value (value is float, integer, string or another container) in one time.\n"
                "Inherits all JValue functions";
        }

        static bool validateReadIndex(const array *obj, UInt32 index) {
            return obj && index < obj->_array.size();
        }

        static bool validateWriteIndex(const array *obj, UInt32 index) {
            return obj && index <= obj->_array.size();
        }

        static void onOutOfBoundAccess() {
            shared_state::instance().setLastError(JError_ArrayOutOfBoundAccess);
        }

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
        REGISTERF(fromArray<SInt32>, "objectWithInts", "values",
"creates new array that contains given values\n\
objectWithBooleans converts booleans into integers");
        REGISTERF(fromArray<BSFixedString>, "objectWithStrings",  "values", NULL);
        REGISTERF(fromArray<Float32>, "objectWithFloats",  "values", NULL);
        REGISTERF(fromArray<bool>, "objectWithBooleans",  "values", NULL);

        static void addFromArray(array *obj, array *another, SInt32 insertAtIndex = -1) {
            if (!obj || !another) {
                return ;
            }

            mutex_lock g1(obj->_mutex), g2(another->_mutex);

            if (insertAtIndex >= 0 && validateWriteIndex(obj, insertAtIndex) == false) {
                onOutOfBoundAccess();
                return;
            }

            SInt32 whereTo = insertAtIndex >= 0 ? insertAtIndex : obj->u_count();

            obj->_array.insert(obj->_array.begin() + whereTo, another->_array.begin(), another->_array.end());
        }
        REGISTERF2(addFromArray, "* sourceArray insertAtIndex=-1",
"adds values from source array into this array. if insertAtIndex is -1 (default behaviour) it adds to the end.\n\
if insertAtIndex >= 0 it appends values starting from insertAtIndex index");

        template<class T>
        static T itemAtIndex(array *obj, Index index) {
            if (!obj) {
                return T(0);
            }

            mutex_lock g(obj->_mutex);
            if (validateReadIndex(obj, index)) {
                return obj->_array[index].readAs<T>();
            } else {
                onOutOfBoundAccess();
                return T(0);
            }
        }
        REGISTERF(itemAtIndex<SInt32>, "getInt", "* index", "returns item at index. getObj function returns container");
        REGISTERF(itemAtIndex<Float32>, "getFlt", "* index", "");
        REGISTERF(itemAtIndex<const char *>, "getStr", "* index", "");
        REGISTERF(itemAtIndex<Handle>, "getObj", "* index", "");
        REGISTERF(itemAtIndex<TESForm*>, "getForm", "* index", "");

        template<class T>
        static SInt32 findVal(array *obj, T value, SInt32 searchStartIndex = 0) {
            if (!obj) {
                return -1;
            }

            mutex_lock g(obj->_mutex);

            if (validateReadIndex(obj, searchStartIndex) == false) {
                onOutOfBoundAccess();
                return -1;
            }

            auto itr = std::find_if(obj->_array.begin() + searchStartIndex, obj->_array.end(), [=](const Item& item) {
                return item.isEqual(value);
            });

            return itr != obj->_array.end() ? (itr - obj->_array.begin()) : -1;
        }
        REGISTERF(findVal<SInt32>, "findInt", "* value searchStartIndex=0",
"returns index of the first found value/container that equals to given value/container (default behaviour if searchStartIndex is 0).\n\
if found nothing returns -1.\n\
searchStartIndex - array index where to start search");
        REGISTERF(findVal<Float32>, "findFlt", "* value searchStartIndex=0", "");
        REGISTERF(findVal<const char *>, "findStr", "* value searchStartIndex=0", "");
        REGISTERF(findVal<object_base*>, "findObj", "* container searchStartIndex=0", "");
        REGISTERF(findVal<TESForm*>, "findForm", "* value searchStartIndex=0", "");

        template<class T>
        static void replaceItemAtIndex(array *obj, Index index, T item) {
            if (!obj) {
                return;
            }

            mutex_lock g(obj->_mutex);
            if (validateReadIndex(obj, index)) {
                obj->_array[index] = Item(item);
            } else {
                onOutOfBoundAccess();
            }
        }
        REGISTERF(replaceItemAtIndex<SInt32>, "setInt", "* index value", "replaces existing value/container at index with new value");
        REGISTERF(replaceItemAtIndex<Float32>, "setFlt", "* index value", "");
        REGISTERF(replaceItemAtIndex<const char *>, "setStr", "* index value", "");
        REGISTERF(replaceItemAtIndex<object_base*>, "setObj", "* index container", "");
        REGISTERF(replaceItemAtIndex<TESForm*>, "setForm", "* index value", "");

        template<class T>
        static void add(array *obj, T item, SInt32 addToIndex = -1) {
            if (!obj) {
                return;
            }

            mutex_lock g(obj->_mutex);
            if (addToIndex >= 0 && validateWriteIndex(obj, addToIndex) == false) {
                onOutOfBoundAccess();
                return;
            }

            SInt32 whereTo = (addToIndex >= 0 ? addToIndex : obj->_array.size());
            obj->_array.insert(obj->_array.begin() + whereTo, Item(item));
        }
        REGISTERF(add<SInt32>, "addInt", "* value addToIndex=-1", "appends value/container to the end of array (default behaviour when addToIndex is -1)\n\
if addToIndex >= 0 it inserts value at given index");
        REGISTERF(add<Float32>, "addFlt", "* value addToIndex=-1", "");
        REGISTERF(add<const char *>, "addStr", "* value addToIndex=-1", "");
        REGISTERF(add<object_base*>, "addObj", "* container addToIndex=-1", "");
        REGISTERF(add<TESForm*>, "addForm", "* value addToIndex=-1", "");

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
                if (validateReadIndex(obj, index)) {
                    obj->_array.erase(obj->_array.begin() + index);
                } else {
                    onOutOfBoundAccess();
                }
            }
        }
        REGISTERF2(eraseIndex, "* index", "erases item at index");

        static bool registerFuncs(VMClassRegistry* registry) {
            bind(registry);
            return true;
        }
    };

};
