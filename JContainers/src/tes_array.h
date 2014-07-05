#include <boost/optional.hpp>

namespace collections {

#define NEGATIVE_IDX_COMMENT "negative index accesses items from the end of array counting backwards."

    class tes_array : public tes_binding::class_meta_mixin_t< tes_array > {
    public:

        REGISTER_TES_NAME("JArray");

        void additionalSetup() {
            metaInfo.comment = "Ordered collection of values (value is float, integer, string or another container).\n"
                "Inherits JValue functionality";
        }

        static bool validateReadIndex(const array *obj, UInt32 index) {
            return obj && index < obj->_array.size();
        }

        static bool validateReadIndexRange(const array *obj, UInt32 begin, UInt32 end) {
            return obj && begin < end && end <= obj->_array.size();
        }

        static bool validateWriteIndex(const array *obj, UInt32 index) {
            return obj && index <= obj->_array.size();
        }

        static void onOutOfBoundAccess() {
            tes_context::instance().setLastError(JError_ArrayOutOfBoundAccess);
        }

        typedef array::Index Index;

        REGISTERF(tes_object::object<array>, "object", "", kCommentObject);

        static object_base* objectWithSize(UInt32 size) {
            auto obj = array::objectWithInitializer([&](array *me) {
                me->_array.resize(size);
            },
                tes_context::instance());

            return obj;
        }
        REGISTERF2(objectWithSize, "size", "creates array of given size, filled with empty items");

        template<class T>
        static object_base* fromArray(VMArray<T> arr) {
            auto obj = array::objectWithInitializer([&](array *me) {
                for (UInt32 i = 0; i < arr.Length(); ++i) {
                    T val;
                    arr.Get(&val, i);
                    me->_array.push_back(Item(tes_binding::convert2J<tes_binding::Tes2J<T>::j_type>(val)));
                }
            },
                tes_context::instance());

            return obj;
        }
        REGISTERF(fromArray<SInt32>, "objectWithInts", "values",
"creates new array that contains given values\n\
objectWithBooleans converts booleans into integers");
        REGISTERF(fromArray<BSFixedString>, "objectWithStrings",  "values", NULL);
        REGISTERF(fromArray<Float32>, "objectWithFloats",  "values", NULL);
        REGISTERF(fromArray<bool>, "objectWithBooleans",  "values", NULL);

        static object_base* subArray(array *source, SInt32 startIndex, SInt32 endIndex) {
            if (!source) {
                return nullptr;
            }

            object_lock g(source);

            if (!validateReadIndexRange(source, startIndex, endIndex)) {
                return nullptr;
            }

            auto obj = array::objectWithInitializer([&](array *me) {
                me->_array.insert(me->begin(), source->begin() + startIndex, source->begin() + endIndex);
            },
                tes_context::instance());

            return obj;
        }
        REGISTERF2(subArray, "* startIndex endIndex", "creates new array containing all values from source array in range [startIndex, endIndex)");

        static void addFromArray(array *obj, array *another, SInt32 insertAtIndex = -1) {
            if (!obj || !another || obj == another) {
                return ;
            }

            object_lock g1(obj), g2(another);

            if (insertAtIndex >= 0 && validateWriteIndex(obj, insertAtIndex) == false) {
                onOutOfBoundAccess();
                return;
            }

            SInt32 whereTo = insertAtIndex >= 0 ? insertAtIndex : obj->u_count();

            obj->_array.insert(obj->begin() + whereTo, another->begin(), another->end());
        }
        REGISTERF2(addFromArray, "* sourceArray insertAtIndex=-1",
"adds values from source array into this array. if insertAtIndex is -1 (default behaviour) it adds to the end.\n\
if insertAtIndex >= 0 it appends values starting from insertAtIndex index");

        typedef boost::optional<uint32> maybe_index;

        static maybe_index convertReadIndex(array *ar, int pyIndex) {
            auto count = ar->u_count();
            uint32 index = (pyIndex >= 0 ? pyIndex : (count + pyIndex) );

            return maybe_index(count > 0 && index < count,
                                index);
        }

        static maybe_index convertWriteIndex(array *ar, int pyIndex) {
            auto count = ar->u_count();
            uint32 index = (pyIndex >= 0 ? pyIndex : (count + pyIndex + 1) );

            return maybe_index(index <= count,
                                index);
        }


        template<class Op>
        static void doReadOp(array *obj, int pyIndex, Op& operation) {
            if (!obj) {
                return;
            }

            object_lock g(obj);

            auto idx = convertReadIndex(obj, pyIndex);

            if (idx) {
                operation(*idx);
            } else {
                onOutOfBoundAccess();
            }
        }

        template<class Op>
        static void doWriteOp(array *obj, int pyIndex, Op& operation) {
            if (!obj) {
                return;
            }

            object_lock g(obj);

            auto idx = convertWriteIndex(obj, pyIndex);

            if (idx) {
                operation(*idx);
            } else {
                onOutOfBoundAccess();
            }
        }

        template<class T>
        static T itemAtIndex(array *obj, Index index) {
            T t((T)0);
            doReadOp(obj, index, [=, &t](uint32 idx) {
                t = obj->_array[idx].readAs<T>();
            });

            return t;
        }
        REGISTERF(itemAtIndex<SInt32>, "getInt", "* index", "returns item at index. getObj function returns container.\n"
            NEGATIVE_IDX_COMMENT);
        REGISTERF(itemAtIndex<Float32>, "getFlt", "* index", "");
        REGISTERF(itemAtIndex<const char *>, "getStr", "* index", "");
        REGISTERF(itemAtIndex<object_base*>, "getObj", "* index", "");
        REGISTERF(itemAtIndex<TESForm*>, "getForm", "* index", "");

        template<class T>
        static SInt32 findVal(array *obj, T value, SInt32 pySearchStartIndex = 0) {

            int result = -1;

            doReadOp(obj, pySearchStartIndex, [=, &result](uint32 idx) {

                auto pred = [=](const Item& item) {
                    return item.isEqual(value);
                };

                if (pySearchStartIndex >= 0) {
                    auto itr = std::find_if(obj->begin() + idx, obj->end(), pred);
                    result = itr != obj->end() ? (itr - obj->begin()) : -1;
                } else {
                    auto itr = std::find_if(obj->rbegin() + (-pySearchStartIndex - 1), obj->rend(), pred);
                    result = itr != obj->rend() ? (obj->rend() - itr) : -1;
                }
            });

            return result;
        }
        REGISTERF(findVal<SInt32>, "findInt", "* value searchStartIndex=0",
"returns index of the first found value/container that equals to given value/container (default behaviour if searchStartIndex is 0).\n\
if found nothing returns -1.\n\
searchStartIndex - array index where to start search\n"
NEGATIVE_IDX_COMMENT);
        REGISTERF(findVal<Float32>, "findFlt", "* value searchStartIndex=0", "");
        REGISTERF(findVal<const char *>, "findStr", "* value searchStartIndex=0", "");
        REGISTERF(findVal<object_base*>, "findObj", "* container searchStartIndex=0", "");
        REGISTERF(findVal<TESForm*>, "findForm", "* value searchStartIndex=0", "");

        template<class T>
        static void replaceItemAtIndex(array *obj, Index index, T item) {
            doReadOp(obj, index, [=](uint32 idx) {
                obj->_array[idx] = Item(item);
            });
        }
        REGISTERF(replaceItemAtIndex<SInt32>, "setInt", "* index value", "replaces existing value/container at index with new value.\n"
                                                                         NEGATIVE_IDX_COMMENT);
        REGISTERF(replaceItemAtIndex<Float32>, "setFlt", "* index value", "");
        REGISTERF(replaceItemAtIndex<const char *>, "setStr", "* index value", "");
        REGISTERF(replaceItemAtIndex<object_base*>, "setObj", "* index container", "");
        REGISTERF(replaceItemAtIndex<TESForm*>, "setForm", "* index value", "");

        template<class T>
        static void addItemAt(array *obj, T item, SInt32 addToIndex = -1) {
            doWriteOp(obj, addToIndex, [&](uint32 idx) {
                obj->_array.insert(obj->begin() + idx, Item(item));
            });
        }
        REGISTERF(addItemAt<SInt32>, "addInt", "* value addToIndex=-1", "appends value/container to the end of array.\n\
if addToIndex >= 0 it inserts value at given index. "NEGATIVE_IDX_COMMENT);
        REGISTERF(addItemAt<Float32>, "addFlt", "* value addToIndex=-1", "");
        REGISTERF(addItemAt<const char *>, "addStr", "* value addToIndex=-1", "");
        REGISTERF(addItemAt<object_base*>, "addObj", "* container addToIndex=-1", "");
        REGISTERF(addItemAt<TESForm*>, "addForm", "* value addToIndex=-1", "");

        static Index count(array *obj) {
            return tes_object::count(obj);
        }
        REGISTERF2(count, "*", "returns number of items in array");

        static void clear(array *obj) {
            tes_object::clear(obj);
        }
        REGISTERF2(clear, "*", "removes all items from array");

        static void eraseIndex(array *obj, SInt32 index) {
            doReadOp(obj, index, [=](uint32 idx) {
                obj->_array.erase(obj->begin() + idx);
            });
        }
        REGISTERF2(eraseIndex, "* index", "erases item at index. "NEGATIVE_IDX_COMMENT);
    };

    TES_META_INFO(tes_array);
};
