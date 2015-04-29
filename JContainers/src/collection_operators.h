#pragma once

#include "collections.h"

#include <thread>
#include "meta.h"
#include "util/istring.h"

namespace collections {

    namespace collection_operators
    {
        using istring = util::istring;
        typedef void (*operator_func)(const Item& item, Item& state);

        struct coll_operator {
            operator_func func;
            const char *func_name;
            const char *description;

            static coll_operator make(operator_func _func, const char *_func_name, const char *_description) {
                coll_operator op = {_func, _func_name, _description};
                return op;
            }
        };

        typedef std::map<istring, coll_operator*> operator_map;

#define COLLECTION_OPERATOR(func, descr) \
    static ::meta<coll_operator> g_collection_operator_##func(coll_operator::make(func, #func, descr));

        template<class Key>
        static coll_operator* get_operator(const Key& key) {
            auto& omap = operators();
            auto itr = omap.find(key);
            return itr != omap.end() ? itr->second : nullptr;
        }

        static operator_map& operators() {

            auto makeOperatorMap = []() -> operator_map {
                operator_map omap;
                for (coll_operator& info : meta<coll_operator>::getListConst()) {
                    omap[info.func_name] = &info;
                }

                return omap;
            };

            static operator_map op_map = makeOperatorMap();
            return op_map;
        }

        void maxNum(const Item& item, Item& state) {
            if (item.isNumber()) {
                state = state.isNull() ? item : Item(
                    (std::max)(item.fltValue(), state.fltValue())
                    );
            }
        }
        COLLECTION_OPERATOR(maxNum, "returns maximum number (int or float) in collection");

        void minNum(const Item& item, Item& state) {
            if (item.isNumber()) {
                state = state.isNull() ? item : Item(
                    (std::min)(item.fltValue(), state.fltValue())
                    );
            }
        }
        COLLECTION_OPERATOR(minNum, "returns minimum number (int or float) in collection");

        void maxFlt(const Item& item, Item& state) {
            if (item.is_type<Item::Real>()) {
                state = state.isNull() ? item : Item(
                    (std::max)(item.fltValue(), state.fltValue())
                    );
            }
        }
        COLLECTION_OPERATOR(maxFlt, "returns maximum float number in collection");

        void minFlt(const Item& item, Item& state) {
            if (item.is_type<Item::Real>()) {
                state = state.isNull() ? item : Item(
                    (std::min)(item.fltValue(), state.fltValue())
                    );
            }
        }
        COLLECTION_OPERATOR(minFlt, "returns minimum float number collection");

        void maxInt(const Item& item, Item& state) {
            if (item.is_type<SInt32>()) {
                state = state.isNull() ? item : Item(
                    (std::max)(item.intValue(), state.intValue())
                    );
            }
        }
        COLLECTION_OPERATOR(maxInt, "returns maximum int number in collection");

        void minInt(const Item& item, Item& state) {
            if (item.is_type<SInt32>()) {
                state = state.isNull() ? item : Item(
                    (std::min)(item.intValue(), state.intValue())
                    );
            }
        }
        COLLECTION_OPERATOR(minInt, "returns minimum int number in collection");


#undef COLLECTION_OPERATOR
    };

}