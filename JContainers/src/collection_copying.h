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

        struct shallow_copy_helper {
            tes_context* _context;

            template<class T> object_base& operator () (const T& origin) const {
                return T::objectWithInitializer([&](T& self) {
                    object_lock lock(origin);
                    self.u_container() = origin.u_container();
                },
                    *_context);
            }
        };

    public:

        static object_base& deep_copy(tes_context& context, const object_base & origin) {
            return deep_copying(context)._deep_copy(origin);
        }

        static object_base& shallow_copy(tes_context& context, const object_base& parent) {
            return perform_on_object_and_return<object_base&>(parent, shallow_copy_helper{ &context });
        }

    private:

        object_base& _deep_copy(const object_base & origin) {
            auto& root_copy = unique_copy(origin);

            object_to_traverse tmp;

            while (!_to_traverse.empty()) {
                tmp.clear();
                tmp.swap(_to_traverse);

                for (auto& obj : tmp) {
                    perform_on_object(*obj, copy_child_objects{ this });
                }
            }

            return root_copy;
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

        struct copy_child_objects {
            deep_copying *const self;
            void operator () (array& ar) {
                object_lock lock(ar);
                for (auto& itm : ar.u_container()) {
                    copy_child(itm);
                }
            }
            template<class T> void operator () (T& map) {
                object_lock lock(map);
                for (auto& pair : map.u_container()) {
                    copy_child(pair.second);
                }
            }
            void copy_child(item& itm) {
                auto origin_child = itm.object();
                if (origin_child) {
                    itm = &self->unique_copy(*origin_child);
                }
            }
        };
    };
}
