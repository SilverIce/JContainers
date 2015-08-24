#pragma once

#include <memory>

#include "meta.h"
#include "util/spinlock.h"
#include "object/object_base.h"
#include "object/object_context.h"

//#include "collections/error_code.h"
#include "collections/collections.h"
#include "collections/dyn_form_watcher.h"

namespace collections
{
    class map;

    class tes_context : public object_context
    {
        using base = object_context;
    public:

        using post_init = ::meta<void(*)(tes_context&)>;

        tes_context()
            : form_watcher(form_watching::dyn_form_watcher::instance())
        {
            for (auto& init : post_init::getListConst()) {
                init(*this);
            }
        }

        ~tes_context() {
            shutdown();
        }

        static tes_context& instance() {
            static tes_context st;
            return st;
        }

    private:

        std::atomic<map*> _cached_root = nullptr;

    public:

        object_stack_ref_template<map> database_ref() {
            return database();
        }

        map* database() {
            map * result = _cached_root.load(std::memory_order_acquire);
            if (!result) {

                spinlock::guard g(_lazyRootInitLock);

                result = _cached_root.load(std::memory_order_relaxed);
                if (!result) {
                    result = base::getObjectOfType<map>(_root_object_id.load(std::memory_order_relaxed));
                    if (!result) {
                        result = &map::object(*this);
                        set_root(result);
                    }

                    _cached_root.store(result, std::memory_order_release);
                }
            }

            return result->as<map>();
        }

    public:

        template<class T>
        T * getObjectOfType(Handle hdl) {
            return getObject(hdl)->as<T>();
        }

        template<class T>
        object_stack_ref_template<T> getObjectRefOfType(Handle hdl) {
            return getObjectRef(hdl)->as<T>();
        }

        // to attach lua context
        std::unique_ptr<dependent_context*>     lua_context;

        form_watching::dyn_form_watcher& form_watcher;

        //////
    public:

        void read_from_stream(std::istream & stream);
        void write_to_stream(std::ostream& stream);

        void read_from_string(const std::string & data);
        std::string write_to_string();

        template<class Archive> void serialize(Archive & ar, const unsigned int version) {
            ar & static_cast<base&>(*this);
            //ar & form_watcher;
        }

        void clearState();
        // complete shutdown, this context shouldn't be used for now
        void shutdown();

    protected:

        void u_clearState() {
            _cached_root = nullptr;
            form_watcher.u_clearState();

            base::u_clearState();
        }

    };

}
