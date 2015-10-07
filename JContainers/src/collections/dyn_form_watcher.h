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

    class form_observer;
    class form_entry;

    using form_entry_ref = boost::shared_ptr < form_entry > ;

    void log(const char* fmt, ...);

    class form_observer {
    private:
        using watched_forms_t = std::hash_map<FormId, boost::weak_ptr<form_entry> >;

        bshared_mutex _mutex;
        watched_forms_t _watched_forms;
        std::atomic_flag _is_inside_unsafe_func;

    public:

        form_observer();

        void on_form_deleted(FormHandle fId);
        form_entry_ref watch_form(FormId fId);

        // Not threadsafe part of API:
        void u_clearState() {
            _watched_forms.clear();
        }

        size_t u_forms_count() const { return _watched_forms.size(); }
        void u_remove_expired_forms();

        /////////////////////////

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive> void save(Archive & ar, const unsigned int version) const {
            ar << _watched_forms;
        }
        template<class Archive> void load(Archive & ar, const unsigned int version) {
            ar >> _watched_forms;
            u_remove_expired_forms();
        }
    };

    class form_ref {

        form_entry_ref _watched_form;

    public:

        form_ref() = default;

        explicit form_ref(const form_entry_ref& entry) : _watched_form(entry) {}
        explicit form_ref(form_entry_ref&& entry) : _watched_form(std::move(entry)) {}

        form_ref(FormId id, form_observer& watcher);
        form_ref(const TESForm& form, form_observer& watcher);

        static form_ref make_expired(FormId formId);

        // Special constructor - to load v <= 3.2.4 data
        enum load_old_id_t { load_old_id };
        explicit form_ref(FormId oldId, form_observer& watcher, load_old_id_t);

        bool is_not_expired() const;
        bool is_expired() const { return !is_not_expired(); }

        FormId get() const;
        FormId get_raw() const;

        bool operator ! () const { return !is_not_expired(); }

        bool operator == (const form_ref& o) const {
            return get() == o.get();
        }
        bool operator != (const form_ref& o) const {
            return !(*this == o);
        }
        bool operator < (const form_ref& o) const;

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive> void save(Archive & ar, const unsigned int version) const;
        template<class Archive> void load(Archive & ar, const unsigned int version);
    };

}

    using form_watching::form_ref;

    template<class Context>
    inline form_ref make_weak_form_id(FormId id, Context& context) {
        return form_ref{ id, *context.form_watcher };
    }

    template<class Context>
    inline form_ref make_weak_form_id(const TESForm* id, Context& context) {
        return id ? form_ref{ *id, *context.form_watcher } : form_ref{};
    }
}
