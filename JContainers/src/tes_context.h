#pragma once

#include "object_base.h"
#include "object_context.h"
#include "tes_error_code.h"
#include "spinlock.h"

#include "collections.h"

namespace collections
{
    class map;

    class tes_context : public object_context
    {
    public:

        static tes_context& instance() {
            static tes_context st;
            return st;
        }

        object_stack_ref_template<map> database_ref() {
            return database();
        }

        map* database() {
            object_base * result = getObject(_root_object_id);

            if (!result) {
                _lazyDBLock.lock();

                result = getObject(_root_object_id);
                if (!result) {
                    result = &map::object(*this);
                    set_root(result);
                }

                _lazyDBLock.unlock();
            }

            return result->as<map>();
        }

        void setDataBase(map *db) {
            set_root(db);
        }

        template<class T>
        T * getObjectOfType(Handle hdl) {
            return getObject(hdl)->as<T>();
        }

        template<class T>
        object_stack_ref_template<T> getObjectRefOfType(Handle hdl) {
            return getObjectRef(hdl)->as<T>();
        }

    };

}
