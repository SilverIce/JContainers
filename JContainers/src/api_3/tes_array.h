#pragma once

#include "collection_functions.h"

namespace tes_api_3 {

    using namespace collections;

#define NEGATIVE_IDX_COMMENT "negative index accesses items from the end of array counting backwards."

    class tes_array : public class_meta< tes_array >, public collections::array_functions {
    public:

        typedef array::ref& ref;

        REGISTER_TES_NAME("JArray");

        void additionalSetup() {
            metaInfo.comment = "Ordered collection of values (value is float, integer, string, form or another container).\n"
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

        typedef array::Index Index;

        REGISTERF(tes_object::object<array>, "object", "", kCommentObject);

        static object_base* objectWithSize(SInt32 size) {
            if (size < 0) {
                return nullptr;
            }

            auto& obj = array::objectWithInitializer([&](array &me) {
                me._array.resize(size);
            },
                tes_context::instance());

            return &obj;
        }
        REGISTERF2(objectWithSize, "size", "creates array of given size, filled with empty items");

        template<class T>
        static object_base* fromArray(VMArray<T> arr) {
            auto obj = &array::objectWithInitializer([&](array &me) {
                for (UInt32 i = 0; i < arr.Length(); ++i) {
                    T val;
                    arr.Get(&val, i);
                    me._array.push_back(Item(val));
                }
            },
                tes_context::instance());

            return obj;
        }
        REGISTERF(fromArray<SInt32>, "objectWithInts", "values",
"creates new array that contains given values\n\
objectWithBooleans converts booleans into integers");
        REGISTERF(fromArray<BSFixedString>, "objectWithStrings",  "values", nullptr);
        REGISTERF(fromArray<Float32>, "objectWithFloats",  "values", nullptr);
        REGISTERF(fromArray<bool>, "objectWithBooleans",  "values", nullptr);

        static object_base* subArray(ref source, SInt32 startIndex, SInt32 endIndex) {
            if (!source) {
                return nullptr;
            }

            object_lock g(source);

            if (!validateReadIndexRange(source.get(), startIndex, endIndex)) {
                return nullptr;
            }

            auto obj = &array::objectWithInitializer([&](array &me) {
                me._array.insert(me.begin(), source->begin() + startIndex, source->begin() + endIndex);
            },
                tes_context::instance());

            return obj;
        }
        REGISTERF2(subArray, "* startIndex endIndex", "creates new array containing all values from source array in range [startIndex, endIndex)");

        static void addFromArray(ref obj, ref another, SInt32 insertAtIndex = -1) {
            if (!obj || !another || obj == another) {
                return ;
            }

            object_lock g2(another);

            doWriteOp(obj, insertAtIndex, [&obj, &another](uint32_t whereTo) {
                obj->_array.insert(obj->begin() + whereTo, another->begin(), another->end());
            });
        }
        REGISTERF2(addFromArray, "* source insertAtIndex=-1",
"adds values from source array into this array. if insertAtIndex is -1 (default behaviour) it adds to the end.\n"
NEGATIVE_IDX_COMMENT);

        static void addFromFormList(ref obj, BGSListForm *formList, SInt32 insertAtIndex = -1) {
            if (!obj || !formList) {
                return;
            }

            struct inserter : BGSListForm::Visitor {

                virtual bool Accept(TESForm * form) override {
                    arr->u_push(Item(form));
                    return false;
                }

                array *arr;

                inserter(array *obj) : arr(obj) {}
            };

            doWriteOp(obj, insertAtIndex, [formList, &obj](uint32_t idx) {
                formList->Visit(inserter(obj.get()));
            });
        }
        REGISTERF2(addFromFormList, "* source insertAtIndex=-1", nullptr);

        template<class T>
        static T itemAtIndex(ref obj, Index index, T t = T(0)) {
            doReadOp(obj, index, [=, &t](uint32_t idx) {
                t = obj->_array[idx].readAs<T>();
            });

            return t;
        }
        REGISTERF(itemAtIndex<SInt32>, "getInt", "* index default=0", "returns item at index. getObj function returns container.\n"
            NEGATIVE_IDX_COMMENT);
        REGISTERF(itemAtIndex<Float32>, "getFlt", "* index default=0.0", "");
        REGISTERF(itemAtIndex<const char *>, "getStr", "* index default=\"\"", "");
        REGISTERF(itemAtIndex<object_base*>, "getObj", "* index default=0", "");
        REGISTERF(itemAtIndex<TESForm*>, "getForm", "* index default=None", "");

        template<class T>
        static SInt32 findVal(ref obj, T value, SInt32 pySearchStartIndex = 0) {

            int result = -1;

            doReadOp(obj, pySearchStartIndex, [=, &result](uint32_t idx) {

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
        static void replaceItemAtIndex(ref obj, Index index, T item) {
            doReadOp(obj, index, [=](uint32_t idx) {
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
        static void addItemAt(ref obj, T item, SInt32 addToIndex = -1) {
            doWriteOp(obj, addToIndex, [&](uint32_t idx) {
                obj->_array.insert(obj->begin() + idx, Item(item));
            });
        }
        REGISTERF(addItemAt<SInt32>, "addInt", "* value addToIndex=-1", "appends value/container to the end of array.\n\
if addToIndex >= 0 it inserts value at given index. "NEGATIVE_IDX_COMMENT);
        REGISTERF(addItemAt<Float32>, "addFlt", "* value addToIndex=-1", "");
        REGISTERF(addItemAt<const char *>, "addStr", "* value addToIndex=-1", "");
        REGISTERF(addItemAt<object_base*>, "addObj", "* container addToIndex=-1", "");
        REGISTERF(addItemAt<TESForm*>, "addForm", "* value addToIndex=-1", "");

        static Index count(ref obj) {
            return tes_object::count(obj);
        }
        REGISTERF2(count, "*", "returns number of items in array");

        static void clear(ref obj) {
            tes_object::clear(obj.to_base<object_base>());
        }
        REGISTERF2(clear, "*", "removes all items from array");

        static void eraseIndex(ref obj, SInt32 index) {
            doReadOp(obj, index, [=](uint32_t idx) {
                obj->_array.erase(obj->begin() + idx);
            });
        }
        REGISTERF2(eraseIndex, "* index", "erases item at index. "NEGATIVE_IDX_COMMENT);

        static SInt32 valueType(ref obj, SInt32 index) {
            SInt32 type = item_type::no_item;
            doReadOp(obj, index, [=, &type](uint32_t idx) {
                type = obj->_array[idx].type();
            });

            return type;
        }
        REGISTERF2(valueType, "* index", "returns type of the value at index. "NEGATIVE_IDX_COMMENT"\n"VALUE_TYPE_COMMENT);


        static void swapItems(ref obj, SInt32 idx, SInt32 idx2) {

            SInt32 pyIndexes[] = { idx, idx2 };
            doReadOp(obj, pyIndexes, [=](const std::array<uint32_t, 2>& indices) {

                if (indices[0] != indices[1]) {
                    std::swap(obj->u_container()[indices[0]], obj->u_container()[indices[1]]);
                }
            });
        }
        REGISTERF2(swapItems, "* index1 index2", "Exchanges the items at index1 and index2. "NEGATIVE_IDX_COMMENT);
    };

    TES_META_INFO(tes_array);
};
