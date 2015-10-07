#pragma once

#include <hash_map>
//#include <memory>
#include <atomic>
#include "boost/shared_ptr.hpp"
#include "boost/smart_ptr/weak_ptr.hpp"
#include "boost/serialization/split_member.hpp"
#include "boost/noncopyable.hpp"

#include "rw_mutex.h"
#include "form_id.h"
#include "util/spinlock.h"
//#include "intrusive_ptr.hpp"

namespace collections {
    
namespace form_watching {

    class dyn_form_watcher;
    class form_entry;

    using form_entry_ref = boost::shared_ptr < form_entry > ;

    void log(const char* fmt, ...);

    class dyn_form_watcher {

        using watched_forms_t = std::hash_map<FormId, boost::weak_ptr<form_entry> >;
    private:
        bshared_mutex _mutex;
        watched_forms_t _watched_forms;
        std::atomic_flag _is_inside_unsafe_func;

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

        void u_remove_expired_forms() const {}
        void u_remove_expired_forms();

    public:

        dyn_form_watcher();

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            u_remove_expired_forms();

            ar & _watched_forms;
        }

        void on_form_deleted(FormHandle fId);
        form_entry_ref watch_form(FormId fId);

        // Not threadsafe part of API:
        void u_clearState() {
            _watched_forms.clear();
        }
        size_t u_forms_count() const { return _watched_forms.size(); }
    };

    class weak_form_id {

        form_entry_ref _watched_form;

    public:

        weak_form_id() = default;

        explicit weak_form_id(const form_entry_ref& entry) : _watched_form(entry) {}
        explicit weak_form_id(form_entry_ref&& entry) : _watched_form(std::move(entry)) {}

        weak_form_id(FormId id, dyn_form_watcher& watcher);
        weak_form_id(const TESForm& form, dyn_form_watcher& watcher);

        static weak_form_id make_expired(FormId formId);

        // Special constructor - to load v <= 3.2.4 data
        enum load_old_id_t { load_old_id };
        explicit weak_form_id(FormId oldId, dyn_form_watcher& watcher, load_old_id_t);

        bool is_not_expired() const;
        bool is_expired() const { return !is_not_expired(); }

        FormId get() const;
        FormId get_raw() const;

        bool operator ! () const { return !is_not_expired(); }

        bool operator == (const weak_form_id& o) const {
            return get() == o.get();
        }
        bool operator != (const weak_form_id& o) const {
            return !(*this == o);
        }
        bool operator < (const weak_form_id& o) const;

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive> void save(Archive & ar, const unsigned int version) const;
        template<class Archive> void load(Archive & ar, const unsigned int version);
    };

}

    using form_watching::weak_form_id;

    template<class Context>
    inline weak_form_id make_weak_form_id(FormId id, Context& context) {
        return weak_form_id{ id, *context.form_watcher };
    }

    template<class Context>
    inline weak_form_id make_weak_form_id(const TESForm* id, Context& context) {
        return id ? weak_form_id{ *id, *context.form_watcher } : weak_form_id{};
    }
}
