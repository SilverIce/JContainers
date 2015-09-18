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
    class watched_form;

    void log(const char* fmt, ...);

    // Had to be single instance as there is single Skyrim instance only?
    class dyn_form_watcher {

        using watched_forms_t = std::hash_map<FormId, boost::weak_ptr<watched_form> >;
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

        void remove_expired_forms() const {}
        void remove_expired_forms();

    public:

        dyn_form_watcher();

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            remove_expired_forms();

            ar & _watched_forms;
        }

        void on_form_deleted(FormHandle fId);
        boost::shared_ptr<watched_form> watch_form(FormId fId);

        // Not threadsafe part of api:
        void u_clearState() {
            _watched_forms.clear();
        }
        size_t u_forms_count() const { return _watched_forms.size(); }
        boost::shared_ptr<watched_form> u_watch_form(FormId fId);
    };

    class weak_form_id {

        FormId _id = FormId::Zero;
        mutable boost::shared_ptr<watched_form> _watched_form;
        mutable bool _expired = true;

    public:

        static weak_form_id make_expired(FormId formId) {
            weak_form_id id;
            id._id = formId;
            id._expired = true;
            return id;
        }

        weak_form_id() {}

        explicit weak_form_id(FormId id, dyn_form_watcher& watcher);

        explicit weak_form_id(const TESForm& form, dyn_form_watcher& watcher);

        // Special constructor - to load v <= 3.2.4 data
        enum load_old_id_t { load_old_id };
        explicit weak_form_id(FormId id, load_old_id_t);

        bool is_not_expired() const;
        bool is_expired() const { return !is_not_expired(); }
        bool is_watched() const { return _watched_form.operator bool(); }

        FormId get() const {
            return is_not_expired() ? _id : FormId::Zero;
        }

        FormId get_raw() const { return _id; }

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
