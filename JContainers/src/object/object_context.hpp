#include "util/util.h"

namespace collections
{
    object_context::object_context()
    {
        registry.reset(new object_registry{});
        aqueue.reset(new autorelease_queue{ *registry });
    }

    object_context::~object_context() {
    }

    object_context::activity_stopper::activity_stopper(object_context& c)
        : context(c)
    {
        c.stop_activity();
    }

    object_context::activity_stopper::~activity_stopper() {
        context.aqueue->start();
    }

    void object_context::stop_activity() {
        aqueue->stop();
    }
    
    void object_context::u_clearState() {
        {
            spinlock::guard g(_dependent_contexts_mutex);
            for (auto& ctx : _dependent_contexts) {
                ctx->clear_state();
            }
        }

        /*  Not good, but working solution.

        purpose: free allocated memory
        problem: a regular delete call won't help as the delete will access memory of possible deleted objects

        solution: isolate the objects by nullifying cross-references, then delete the objects

        actually all I need is just free all allocated memory, but this is hardly achievable

        */
        {
            aqueue->u_nullify();

            for (auto& obj : registry->u_all_objects()) {
                obj->u_nullifyObjects();
            }
            for (auto& obj : registry->u_all_objects()) {
                delete obj;
            }

            registry->u_clear();
            aqueue->u_clear();
        }
    }

    std::vector<object_stack_ref> object_context::filter_objects(std::function<bool(object_base& obj)> predicate) const {
        return registry->filter_objects(predicate);
    }

    object_base * object_context::getObject(Handle hdl) {
        return registry->getObject(hdl);
    }

    object_stack_ref object_context::getObjectRef(Handle hdl) {
        return registry->getObjectRef(hdl);
    }

    object_base * object_context::u_getObject(Handle hdl) {
        return registry->u_getObject(hdl);
    }

    size_t object_context::aqueueSize() const {
        return aqueue->count();
    }

    size_t object_context::object_count() const {
        return registry->object_count();
    }

    size_t object_context::collect_garbage() {
        activity_stopper s{ *this };
        auto res = garbage_collector::u_collect(*registry, *aqueue);
        return res.garbage_total;
    }

    //////////////////////////////////////////////////////////////////////////

    template<>
    void object_context::load(boost::archive::binary_iarchive & ar, unsigned int version) {
        ar >> *registry >> *aqueue;
    }

    template<>
    void object_context::save(boost::archive::binary_oarchive & ar, unsigned int version) const {
        ar << *registry << *aqueue;
    }

    template<>
    void object_context::load_data_in_old_way(boost::archive::binary_iarchive& ar) {
        ar >> *registry >> *aqueue;
    }

    void object_context::u_print_stats() const {
        JC_log("%lu objects total", registry->u_all_objects().size());
        JC_log("%lu public objects", registry->u_public_object_count());
        JC_log("%lu objects in aqueue", aqueue->u_count());
    }

    //////////////////////////////////////////////////////////////////////////

    void object_context::u_postLoadInitializations() {
        for (auto& obj : registry->u_all_objects()) {
            obj->set_context(*this);
        }
        for (auto& obj : registry->u_all_objects()) {
            obj->u_onLoaded();
        }
    }

    void object_context::u_postLoadMaintenance(const serialization_version saveVersion)
    {
        util::do_with_timing("Garbage collection", [&]() {
            auto res = garbage_collector::u_collect(*registry, *aqueue);
            JC_log("%u garbage objects collected. %u objects are parts of cyclic graphs", res.garbage_total, res.part_of_graphs);
        });
    }

    void object_context::add_dependent_context(dependent_context& ctx) {
        spinlock::guard g(_dependent_contexts_mutex);
        if (std::find(_dependent_contexts.begin(), _dependent_contexts.end(), &ctx) == _dependent_contexts.end()) {
            _dependent_contexts.push_back(&ctx);
        }
    }

    void object_context::remove_dependent_context(dependent_context& ctx) {
        spinlock::guard g(_dependent_contexts_mutex);
        _dependent_contexts.erase(std::remove(_dependent_contexts.begin(), _dependent_contexts.end(), &ctx), _dependent_contexts.end());
    }


}
