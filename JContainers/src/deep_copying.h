#pragma once

#include <set>
#include "collections.h"

namespace collections {

    class deep_copying {
        // original - copy
        typedef std::map<const object_base *, object_base *> copyed_objects;

        typedef std::vector<object_base *> object_to_traverse;

        tes_context& _context;
        copyed_objects _copyed;
        object_to_traverse _to_traverse;

        deep_copying(tes_context& context) : _context(context) {}

    public:

        static object_base& deep_copy(tes_context& context, const object_base & origin) {
            return deep_copying(context)._deep_copy(origin);
        }

        static object_base& shallow_copy(tes_context& context, const object_base& parent) {
            object_lock lock(parent);
            if (auto obj = parent.as<array>()) {
                return shallow_copy_helper(context, *obj);
            }
            else if (auto obj = parent.as<map>()) {
                return shallow_copy_helper(context, *obj);
            }
            else if (auto obj = parent.as<form_map>()) {
                return shallow_copy_helper(context, *obj);
            }
            assert(false);
            return *(object_base*)nullptr;
        }

    private:

        object_base& _deep_copy(const object_base & origin) {
            auto& root_copy = unique_copy(origin);

            object_to_traverse tmp;

            while (!_to_traverse.empty()) {
                tmp.clear();
                tmp.swap(_to_traverse);

                for (auto& obj : tmp) {
                    copy_childs(*obj);
                }
            }

            return root_copy;
        }

        template<class T>
        static object_base& shallow_copy_helper(tes_context& context, const T& origin) {
            return T::objectWithInitializer([&](T& copy) {
                copy.u_container() = origin.u_container();
            },
                context);
        }

        object_base& unique_copy(const object_base & origin) {
            auto itr = _copyed.find(&origin);
            if (itr != _copyed.end()) {
                return *itr->second;
            }
            else {
                auto& copy = shallow_copy(_context, origin);
                _to_traverse.push_back(&copy);
                _copyed[&origin] = &copy;
                return copy;
            }
        }

        void copy_childs(object_base& parent) {
            object_lock lock(parent);
            if (auto obj = parent.as<array>()) {
                for (auto& itm : obj->u_container()) {
                    copy_child(itm);
                }
            }
            else if (auto obj = parent.as<map>()) {
                for (auto& pair : obj->u_container()) {
                    copy_child(pair.second);
                }
            }
            else if (auto obj = parent.as<form_map>()) {
                for (auto& pair : obj->u_container()) {
                    copy_child(pair.second);
                }
            }
            else
                assert(false);
        }

        void copy_child(Item& itm) {
            auto origin_child = itm.object();
            if (origin_child) {
                itm = &unique_copy(*origin_child);
            }
        }
    };
}
