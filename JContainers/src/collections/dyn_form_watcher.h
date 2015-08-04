#pragma once

#include <hash_map>
//#include <memory>
#include <atomic>
#include "boost/shared_ptr.hpp"
#include "boost/serialization/split_member.hpp"

#include "rw_mutex.h"
#include "form_id.h"
#include "util/spinlock.h"
//#include "intrusive_ptr.hpp"

namespace collections {
    
namespace form_watching {

    class dyn_form_watcher;

    class watched_form {
    public:

        std::atomic<bool> _deleted;

        watched_form() {
            _deleted._My_val = false; // hack
        }

        bool is_deleted() const {
            return _deleted.load(std::memory_order_relaxed);
        }

        void set_deleted() {
            _deleted.store(true, std::memory_order_relaxed);
        }
    };

    // - remove FormID if none watches it
    // - 

    // Had to be single instance as there is single Skyrim instance only?
    class dyn_form_watcher {

        using watched_forms_t = std::hash_map<FormId, boost::shared_ptr<watched_form> >;
    public:

        bshared_mutex _mutex;
        watched_forms_t _watched_forms;

        std::atomic_flag _is_inside_unsafe_func;

    public:

        dyn_form_watcher() {
            _is_inside_unsafe_func._My_flag = false;
        }

        static dyn_form_watcher& instance() {
            static dyn_form_watcher _inst;
            return _inst;
        }

        template<class ReadCondition, class WriteAction, class Target>
        static bool if_condition_then_perform(ReadCondition& condition, WriteAction& action, Target& target) {
            bool condition_met = false;
            {
                read_lock r(target._mutex);
                condition_met = condition(const_cast<const Target&>(target));
            }

            if (condition_met) {
                write_lock w(target._mutex);
                action(target);
            }

            return condition_met;
        }

        void on_form_deleted(FormId fId);

        void u_clearState() {
            _watched_forms.clear();
        }

        boost::shared_ptr<watched_form> watch_form(FormId fId);

        boost::shared_ptr<watched_form> u_watch_form(FormId fId);
    };

    class weak_form_id {

        FormId _id = FormId::Zero;
        mutable boost::shared_ptr<watched_form> _watched_form;
        mutable bool _expired = true;

    public:

        weak_form_id() {}

        explicit weak_form_id(FormId id);

        explicit weak_form_id(const TESForm& form);

        // Special constructor - to load v <= 3.2.4 data
        enum load_old_id_t { load_old_id };
        explicit weak_form_id(FormId id, load_old_id_t) {
            u_load_old_form_id(id);
        }

        bool is_not_expired() const;

        FormId get() const {
            return is_not_expired() ? _id : FormId::Zero;
        }

        bool operator ! () const { return !is_not_expired(); }

        bool operator == (const weak_form_id& o) const {
            return _id == o._id;
        }
        bool operator != (const weak_form_id& o) const {
            return !(*this == o);
        }
        bool operator < (const weak_form_id& o) const {
            return _id < o._id;
        }

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive> void save(Archive & ar, const unsigned int version) const;

        template<class Archive> void load(Archive & ar, const unsigned int version);

    private:

        void u_load_old_form_id(FormId oldId);
    };

}

    using form_watching::weak_form_id;

    template<class Context>
    inline weak_form_id make_weak_form_id(FormId id, Context& context) {
        return weak_form_id(id);
    }

    template<class Context>
    inline weak_form_id make_weak_form_id(const TESForm* id, Context& context) {
        return id ? weak_form_id(*id) : weak_form_id();
    }

}
