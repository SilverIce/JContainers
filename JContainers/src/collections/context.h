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

        void u_print_stats() const;
        void u_applyUpdates(const serialization_version saveVersion);

    public:

        using post_init = ::meta<void(*)(tes_context&)>;

        tes_context()
            : form_watcher(new form_watching::form_observer{})
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
        std::atomic<Handle> _root_object_id{ Handle::Null };
        spinlock _lazyRootInitLock;

    public:

        void set_root(object_base *db);
        map& root();

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
        std::shared_ptr<dependent_context>     lua_context;

        std::shared_ptr<form_watching::form_observer> form_watcher;

        //////
    public:

        void read_from_stream(std::istream & stream);
        void write_to_stream(std::ostream& stream);

        void read_from_string(const std::string & data);
        std::string write_to_string();

        template<class Archive> void load(Archive & ar, unsigned int version);
        template<class Archive> void save(Archive & ar, unsigned int version) const;
        template<class Archive> void load_data_in_old_way(Archive& ar);

        void clearState();
        // complete shutdown, this context shouldn't be used for now
        void shutdown();

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

    protected:

        void u_clearState() {
            _root_object_id.store(Handle::Null, std::memory_order_relaxed);
            _cached_root = nullptr;
            form_watcher->u_clearState();

            base::u_clearState();
        }

    };

    // so that this won't be lost or hidden
    inline tes_context& HACK_get_tcontext(const object_base& obj) {
        return static_cast<tes_context&>(obj.context());
    }

}
