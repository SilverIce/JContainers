#pragma once

//#include <hash_map>
#include <concurrent_unordered_map.h>
//#include <memory>
#include <atomic>
#include <tuple>
#include "boost/shared_ptr.hpp"
#include "boost/smart_ptr/weak_ptr.hpp"
#include "boost/serialization/split_member.hpp"
#include "boost/noncopyable.hpp"

#include "util/spinlock.h"

#include "rw_mutex.h"
#include "form_id.h"

namespace collections {
    
namespace form_watching {

    class form_observer;
    class form_entry;

    using form_entry_ref = boost::shared_ptr < form_entry > ;

    class form_observer {
    private:
        using weak_entry = boost::weak_ptr<form_entry>;
       // using watched_forms_old_t = std::hash_map<FormId, weak_entry >;
        using watched_forms_t = concurrency::concurrent_unordered_map < FormId, weak_entry >;

        //bshared_mutex _mutex;
        watched_forms_t _watched_forms;

    public:

        form_observer() = default;

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

        template<class Archive> void save(Archive & ar, const unsigned int version) const;
        template<class Archive> void load(Archive & ar, const unsigned int version);
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

        // "Stupid" comparison operators, compare identifiers:
        // the functions don't care whether the @form_refs are really equal or not -
        // really equal form_refs have equal @_watched_form field
        // The comparison is NOT stable
        bool operator == (const form_ref& o) const {
            return get() == o.get();
        }
        bool operator != (const form_ref& o) const {
            return !(*this == o);
        }
        bool operator < (const form_ref& o) const {
            return get() < o.get();
        }

        // Implements stable 'less than' comparison
        struct stable_less_comparer;

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive> void save(Archive & ar, const unsigned int version) const;
        template<class Archive> void load(Archive & ar, const unsigned int version);
    };

    struct form_ref::stable_less_comparer {
        bool operator () (const form_ref& left, const form_ref& right) const {
            auto leftId = left.get_raw(), rightId = right.get_raw();
            return std::tie(leftId, left._watched_form) < std::tie(rightId, right._watched_form);
        }
    };

}

    using form_watching::form_ref;

    template<class Context>
    inline form_ref make_weak_form_id(FormId id, Context& context) {
        return form_ref{ id, context._form_watcher };
    }

    template<class Context>
    inline form_ref make_weak_form_id(const TESForm* id, Context& context) {
        return id ? form_ref(*id, context._form_watcher) : form_ref();
    }
}
