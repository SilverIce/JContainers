#pragma once

#include <atomic>
#include <functional>
#include <deque>
#include <boost/serialization/split_member.hpp>

#include "object_base.h"

namespace boost {
namespace archive {
    class binary_iarchive;
    class binary_oarchive;
}
}

namespace collections {

    class object_registry;
    class autorelease_queue;


    class dependent_context {
    public:
        virtual ~dependent_context() {}
        virtual void clear_state() = 0;
    };


    enum class serialization_version {
        pre_aqueue_fix = 2,
        no_header = 3, // no JSON header in the beginning of a stream
        pre_gc = 4, // next version implements GC
        pre_dyn_form_watcher = 5, // next version implements dynamic-form-watcher
        current = 6,
    };

    /*
    struct object_context_delegate
    {
        virtual void u_loadAdditional(boost::archive::binary_iarchive & arch) = 0;
        virtual void u_saveAdditional(boost::archive::binary_oarchive & arch) = 0;
        virtual void u_cleanup() = 0;
        virtual void u_applyUpdates(const serialization_version saveVersion) {}
    };
    */

    class object_context {

    protected:
        void u_postLoadInitializations();
        void u_applyUpdates(const serialization_version saveVersion);
        void u_postLoadMaintenance(const serialization_version saveVersion);
        void u_print_stats() const;

    public:
        object_registry* registry = nullptr;
        autorelease_queue* aqueue = nullptr;

    public:

        object_context();
        ~object_context();

        std::vector<object_stack_ref> filter_objects(std::function<bool(object_base& obj)> predicate) const;

        template<class T>
        T * getObjectOfType(Handle hdl) {
            return getObject(hdl)->as<T>();
        }

        template<class T>
        object_stack_ref_template<T> getObjectRefOfType(Handle hdl) {
            return getObjectRef(hdl)->as<T>();
        }

        size_t aqueueSize();
        object_base * getObject(Handle hdl);
        object_stack_ref getObjectRef(Handle hdl);
        object_base * u_getObject(Handle hdl);

        // exposed for testing purposes only
        size_t collect_garbage();
    public:

        // stops object_context's activity, until destroyed and then restarts it 
        struct activity_stopper {
            object_context& context;
            activity_stopper(object_context& context);
            ~activity_stopper();
        };

        void stop_activity();
        void u_clearState();

    public:

        template<class Archive>
        void load(Archive & ar, unsigned int version);
        template<class Archive>
        void save(Archive & ar, unsigned int version) const;

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

    protected:
        std::atomic<Handle> _root_object_id{ HandleNull };
        spinlock _lazyDBLock;

    public:
        Handle root_id() const { return _root_object_id;}

        object_base* root();
        void set_root(object_base *db);

    private:
        spinlock _dependent_contexts_mutex;
        std::vector<dependent_context*> _dependent_contexts;

    public:
        void add_dependent_context(dependent_context& ctx);
        void remove_dependent_context(dependent_context& ctx);

    };

}

