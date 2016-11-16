#pragma once

#include "collections/functions.h"

namespace tes_api_3 {

    using namespace collections;

#define NEGATIVE_IDX_COMMENT "negative index accesses items from the end of container counting backwards."

    class tes_array : public class_meta< tes_array >, public collections::array_functions {
    public:

        typedef array* ref;

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

        static object_base* objectWithSize(tes_context& ctx, SInt32 size) {
            if (size < 0) {
                return nullptr;
            }

            auto& obj = array::objectWithInitializer([&](array &me) {
                me._array.resize(size);
            },
                ctx);

            return &obj;
        }
        REGISTERF2(objectWithSize, "size", "Creates a new array of given size, filled with empty (None) items");

        template<class TesType, class JCType = TesType>
        static object_base* fromArray(tes_context& ctx, VMArray<TesType> arr) {
            auto obj = &array::objectWithInitializer([&](array &me) {
                me.u_container().reserve(arr.Length());
                for (UInt32 i = 0; i < arr.Length(); ++i) {
                    TesType val;
                    arr.Get(&val, i);
                    me.u_container().emplace_back(reflection::binding::get_converter<JCType>::convert2J(val, ctx));
                }
            },
                ctx);

            return obj;
        }
        REGISTERF(fromArray<SInt32>, "objectWithInts", "values",
"Creates a new array that contains given values\n\
objectWithBooleans converts booleans into integers");
        REGISTERF(fromArray<skse::string_ref>, "objectWithStrings",  "values", nullptr);
        REGISTERF(fromArray<Float32>, "objectWithFloats",  "values", nullptr);
        REGISTERF(fromArray<bool>, "objectWithBooleans",  "values", nullptr);
        REGISTERF(ARGS(fromArray<TESForm*, form_ref>), "objectWithForms", "values", nullptr);

        static object_base* subArray(tes_context& ctx, ref source, SInt32 startIndex, SInt32 endIndex) {
            if (!source) {
                return nullptr;
            }

            object_lock g(source);

            if (!validateReadIndexRange(source, startIndex, endIndex)) {
                return nullptr;
            }

            auto obj = &array::objectWithInitializer([&](array &me) {
                me._array.insert(me.begin(), source->begin() + startIndex, source->begin() + endIndex);
            },
                ctx);

            return obj;
        }
        REGISTERF2(subArray, "* startIndex endIndex", "Creates a new array containing all the values from the source array in range [startIndex, endIndex)");

        static void addFromArray(tes_context& ctx, ref obj, ref another, SInt32 insertAtIndex = -1) {
            if (!obj || !another || obj == another) {
                return ;
            }

            object_lock g2(another);

            doWriteOp(obj, insertAtIndex, [&obj, &another](uint32_t whereTo) {
                obj->_array.insert(obj->begin() + whereTo, another->begin(), another->end());
            });
        }
        REGISTERF2(addFromArray, "* source insertAtIndex=-1",
"Inserts the values from the source array into this array. If insertAtIndex is -1 (default behaviour) it appends to the end.\n"
NEGATIVE_IDX_COMMENT);

        static void addFromFormList(tes_context& ctx, ref obj, BGSListForm *formList, SInt32 insertAtIndex = -1) {
            if (!obj || !formList) {
                return;
            }

            struct inserter : BGSListForm::Visitor {

                virtual bool Accept(TESForm * form) override {
                    arr->u_container().insert(arr->u_container().begin() + insertIdx, item{ make_weak_form_id(form, context) });
                    return false;
                }

                array *arr;
                uint32_t insertIdx;
                tes_context& context;

                inserter(array *obj, uint32_t insertAt, tes_context& c) : arr(obj), insertIdx(insertAt), context(c) {}
            };

            doWriteOp(obj, insertAtIndex, [formList, &obj, &ctx](uint32_t idx) {
                formList->Visit(inserter{ obj, idx, ctx });
            });
        }
        REGISTERF2(addFromFormList, "* source insertAtIndex=-1", nullptr);

        template<class T>
        static T itemAtIndex(tes_context& ctx, ref obj, Index index, T t = default_value<T>()) {
            doReadOp(obj, index, [=, &t](uint32_t idx) {
                t = obj->_array[idx].readAs<T>();
            });

            return t;
        }
        REGISTERF(itemAtIndex<SInt32>, "getInt", "* index default=0", "Returns the item at the index of the array.\n"
            NEGATIVE_IDX_COMMENT);
        REGISTERF(itemAtIndex<Float32>, "getFlt", "* index default=0.0", "");
        REGISTERF(itemAtIndex<skse::string_ref>, "getStr", "* index default=\"\"", "");
        REGISTERF(itemAtIndex<object_base*>, "getObj", "* index default=0", "");
        REGISTERF(itemAtIndex<form_ref>, "getForm", "* index default=None", "");

        template<class T>
        static SInt32 findVal(tes_context& ctx, ref obj, T value, SInt32 pySearchStartIndex = 0) {

            int result = -1;

            doReadOp(obj, pySearchStartIndex, [=, &result](uint32_t idx) {
                if (pySearchStartIndex >= 0) {
                    auto itr = std::find(obj->begin() + idx, obj->end(), item(value));
                    result = itr != obj->end() ? (itr - obj->begin()) : -1;
                } else {
                    auto itr = std::find(obj->rbegin() + (-pySearchStartIndex - 1), obj->rend(), item(value));
                    result = itr != obj->rend() ? (obj->rend() - itr) : -1;
                }
            });

            return result;
        }
        REGISTERF(findVal<SInt32>, "findInt", "* value searchStartIndex=0",
"Returns the index of the first found value/container that equals to given the value/container (default behaviour if searchStartIndex is 0).\n\
If nothing was found it returns -1.\n\
@searchStartIndex - index of the array where to start search\n"
NEGATIVE_IDX_COMMENT);
        REGISTERF(findVal<Float32>, "findFlt", "* value searchStartIndex=0", "");
        REGISTERF(findVal<const char *>, "findStr", "* value searchStartIndex=0", "");
        REGISTERF(findVal<object_base*>, "findObj", "* container searchStartIndex=0", "");
        REGISTERF(findVal<form_ref>, "findForm", "* value searchStartIndex=0", "");

        template<class T>
        static void replaceItemAtIndex(tes_context& ctx, ref obj, Index index, T val) {
            doReadOp(obj, index, [=](uint32_t idx) {
                obj->_array[idx] = item(val);
            });
        }
        REGISTERF(replaceItemAtIndex<SInt32>, "setInt", "* index value", "Replaces existing value at the @index of the array with the new @value.\n"
                                                                         NEGATIVE_IDX_COMMENT);
        REGISTERF(replaceItemAtIndex<Float32>, "setFlt", "* index value", "");
        REGISTERF(replaceItemAtIndex<const char *>, "setStr", "* index value", "");
        REGISTERF(replaceItemAtIndex<object_base*>, "setObj", "* index container", "");
        REGISTERF(replaceItemAtIndex<form_ref>, "setForm", "* index value", "");

        template<class T>
        static void addItemAt(tes_context& ctx, ref obj, T val, SInt32 addToIndex = -1) {
            doWriteOp(obj, addToIndex, [&](uint32_t idx) {
                (void)obj->_array.emplace(obj->begin() + idx, val);
            });
        }
        REGISTERF(addItemAt<SInt32>, "addInt", "* value addToIndex=-1", "Appends the @value/@container to the end of the array.\n\
If @addToIndex >= 0 it inserts value at given index. " NEGATIVE_IDX_COMMENT);
        REGISTERF(addItemAt<Float32>, "addFlt", "* value addToIndex=-1", "");
        REGISTERF(addItemAt<const char *>, "addStr", "* value addToIndex=-1", "");
        REGISTERF(addItemAt<object_base*>, "addObj", "* container addToIndex=-1", "");
        REGISTERF(addItemAt<form_ref>, "addForm", "* value addToIndex=-1", "");

        static Index count(tes_context& ctx, ref obj) {
            return tes_object::count(ctx, obj);
        }
        REGISTERF2(count, "*", "Returns count of the items in the array");

        static void clear(tes_context& ctx, ref obj) {
            tes_object::clear(ctx, obj);
        }
        REGISTERF2(clear, "*", "Removes all the items from the array");

        static void eraseIndex(tes_context& ctx, ref obj, SInt32 index) {
            doReadOp(obj, index, [=](uint32_t idx) {
                obj->_array.erase(obj->begin() + idx);
            });
        }
        REGISTERF2(eraseIndex, "* index", "Erases the item at the index. "NEGATIVE_IDX_COMMENT);

        static void eraseRange(tes_context& ctx, ref obj, SInt32 first, SInt32 last) {

            // 0,1,2,3,4
            // b        e
            // -1 is 4th index
            // begin + 4 is last, valid iterator
            SInt32 pyIndexes[] { first, last };
            doReadOp(obj, pyIndexes, [=](const std::array<uint32_t, 2>& indices) {
                if (indices[0] <= indices[1]) {
                    obj->_array.erase(obj->begin() + indices[0], obj->begin() + indices[1] + 1);
                }
            });
        }
        REGISTERF2(eraseRange, "* first last", "Erases [first, last] index range of the items. "NEGATIVE_IDX_COMMENT
            "\nFor ex. with [1,-1] range it will erase everything except the first item");

        static SInt32 valueType(tes_context& ctx, ref obj, SInt32 index) {
            SInt32 type = item_type::no_item;
            doReadOp(obj, index, [=, &type](uint32_t idx) {
                type = obj->_array[idx].type();
            });

            return type;
        }
        REGISTERF2(valueType, "* index", "Returns type of the value at the @index. "NEGATIVE_IDX_COMMENT"\n"VALUE_TYPE_COMMENT);

        static void swapItems(tes_context& ctx, ref obj, SInt32 idx, SInt32 idx2) {

            SInt32 pyIndexes[] = { idx, idx2 };
            doReadOp(obj, pyIndexes, [=](const std::array<uint32_t, 2>& indices) {

                if (indices[0] != indices[1]) {
                    std::swap(obj->u_container()[indices[0]], obj->u_container()[indices[1]]);
                }
            });
        }
        REGISTERF2(swapItems, "* index1 index2", "Exchanges the items at @index1 and @index2. "NEGATIVE_IDX_COMMENT);

        static ref sort(tes_context& ctx, ref obj) {
            if (obj) {
                object_lock g(obj);
                std::sort(obj->u_container().begin(), obj->u_container().end());
            }
            return obj;
        }
        REGISTERF2(sort, "*", "Sorts the items into ascending order (none < int < float < form < object < string). Returns the array itself");

        static ref unique(tes_context& ctx, ref obj) {
            if (obj) {
                object_lock g(obj);
                std::sort(obj->u_container().begin(), obj->u_container().end());
                auto newEnd = std::unique(obj->u_container().begin(), obj->u_container().end());
                obj->u_container().erase(newEnd, obj->u_container().end());
            }
            return obj;
        }
        REGISTERF2(unique, "*", "Sorts the items, removes duplicates. Returns array itself. You can treat it as JSet now");

        template<
            typename ValueType,
            typename PArrayType = VMArray<ValueType>
        >
        static bool writeToPapyrusArray(
                tes_context& ctx
                , ref obj
                , PArrayType targetArray
                , SInt32 py_writeAtIdx = 0         // start write at index of @targetArray
                , SInt32 py_stopWriteAtIdx = -1    // stop writing at this last index of @targetArray
                , SInt32 py_readIdx = 0
                , ValueType defaultRead = default_value<ValueType>()
            )
        {
            if (!obj) {
                return false;
            }

            const auto targetLength = targetArray.Length();
            const auto fstIdx = convertReadIndex(targetLength, py_writeAtIdx);
            const auto lstIdx = convertReadIndex(targetLength, py_stopWriteAtIdx);

            if (!(fstIdx && lstIdx && *fstIdx <= *lstIdx)) {
                return false;
            }

            object_lock l(obj);
            const auto readIdx = convertReadIndex(obj, py_readIdx);
            const int32_t countToRead = *lstIdx - *fstIdx + 1;

            if (!(readIdx && (*readIdx + countToRead - 1) < obj->u_count())) {
                return false;
            }

            for (int32_t i = 0; i < countToRead; ++i) {
                ValueType value = (*obj)[i + *readIdx].readAs<ValueType>();
                targetArray.Set(&value, i + *fstIdx);
            }

            return true;
        }

#define ARGNAMES "* targetArray writeAtIdx=0 stopWriteAtIdx=-1 readIdx=0 defaultRead="
        REGISTERF(writeToPapyrusArray<SInt32>, "writeToIntegerPArray", ARGNAMES "0", "TOTOTO??");
        REGISTERF(writeToPapyrusArray<Float32>, "writeToFloatPArray", ARGNAMES "0.0", "");
        REGISTERF(writeToPapyrusArray<TESForm*>, "writeToFormPArray", ARGNAMES "None", "");
        //REGISTERF(writeToPapyrusArray<bool>, "writeToBooleanPArray", ARGNAMES, "");
        REGISTERF(writeToPapyrusArray<skse::string_ref>, "writeToStringPArray", ARGNAMES "\"\"", "");

#undef ARGNAMES

    };

    TES_META_INFO(tes_array);
};
