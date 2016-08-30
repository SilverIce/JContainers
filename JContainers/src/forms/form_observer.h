#pragma once

//#include <hash_map>
#include <concurrent_unordered_map.h>
//#include <memory>
#include <atomic>
#include <tuple>
#include <assert.h>
#include "boost/shared_ptr.hpp"
#include "boost/smart_ptr/weak_ptr.hpp"
#include "boost/serialization/split_member.hpp"
#include "boost/serialization/version.hpp"
#include "boost/noncopyable.hpp"
#include "boost/core/explicit_operator_bool.hpp"

#include "util/spinlock.h"
#include "util/stl_ext.h"

#include "rw_mutex.h"
#include "form_id.h"

class TESForm;

namespace forms {

    class form_observer;
    class form_entry;

    using form_entry_ref = boost::shared_ptr < form_entry > ;

    class form_observer {
    private:
        using weak_entry = boost::weak_ptr<form_entry>;
        using watched_forms_t = concurrency::concurrent_unordered_map < FormId, weak_entry >;

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
        void u_print_status() const;

        /////////////////////////

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive> void save(Archive & ar, const unsigned int version) const;
        template<class Archive> void load(Archive & ar, const unsigned int version);
    };

    class form_ref {
        form_entry_ref _watched_form;

    public:

        explicit form_ref(form_entry_ref&& entry) : _watched_form(std::move(entry)) {}
        explicit form_ref(const form_entry_ref& entry) : _watched_form(entry) {}

        form_ref& operator = (form_ref&& entry) {
            if (this != &entry) {
                _watched_form = std::move(entry._watched_form);
            }
            return *this;
        }

        form_ref() = default;
        form_ref& operator = (const form_ref &) = default;

        form_ref(FormId id, form_observer& watcher);
        form_ref(const TESForm& form, form_observer& watcher);

        static form_ref make_expired(FormId formId);

        // Special constructor - to load pre v3.3 data
        enum load_old_id_t { load_old_id };
        explicit form_ref(FormId oldId, form_observer& watcher, load_old_id_t);

        bool is_not_expired() const;
        bool is_expired() const { return !is_not_expired(); }

        FormId get() const;
        FormId get_raw() const;

        bool operator!() const BOOST_NOEXCEPT { return is_expired(); }
        BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT();

        void swap(form_ref& other) {
            static_assert(sizeof(other) == sizeof(_watched_form),
                "ensures that no additional fields were added");
            _watched_form.swap(other._watched_form);
        }

        // Implements stable 'less than' comparison
        struct stable_less_comparer;

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive> void save(Archive & ar, const unsigned int version) const;
        template<class Archive> void load(Archive & ar, const unsigned int version);
    };

    struct form_ref::stable_less_comparer {
        template<class FormRef1, class FormRef2>
        bool operator () (const FormRef1& left, const FormRef2& right) const {
            return std::make_tuple(left.get_raw(), left.is_expired())
                < std::make_tuple(right.get_raw(), right.is_expired());
        }
    };

    // It's lightweight alternative to form_ref to temporarily hold forms
    // why lightweight? form_ref constructor accesses form_observer, which is costly
    class form_ref_lightweight {
        FormId _formId = FormId::Zero;
        form_observer* _observer = nullptr;

    public:

        form_ref_lightweight(FormId id, form_observer& watcher)
            : _formId(id), _observer(&watcher) {}

        // allow implicit conversion
        form_ref_lightweight(const form_ref& ref)
            : _formId(ref.get()) {}

        form_ref_lightweight() = default;

        form_ref to_form_ref() const {
            return is_not_expired() ? (assert(_observer), form_ref(_formId, *_observer)) : form_ref();
        }

        // mimic form_ref interface
        FormId get() const { return _formId; }
        FormId get_raw() const { return _formId; }

        bool operator!() BOOST_CONSTEXPR_OR_CONST BOOST_NOEXCEPT{ return is_expired(); }
        BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()

        bool is_expired() BOOST_CONSTEXPR_OR_CONST{ return _formId == FormId::Zero; }
        bool is_not_expired() BOOST_CONSTEXPR_OR_CONST{ return !is_expired(); }
    };

    // "Stupid" form_ref comparison functions:
    // the functions don't care whether the @form_refs are really equal or not -
    // really equal form_refs have equal @_watched_form fields
    // The comparison is NOT stable
    namespace comp {
        template<class FormRef1, class FormRef2>
        inline bool equal(const FormRef1& left, const FormRef2& right) {
            return left.get() == right.get();
        }
        template<class FormRef1, class FormRef2>
        inline bool less(const FormRef1& left, const FormRef2& right) {
            return left.get() < right.get();
        }
    }

    template<class FormRef>
    inline bool operator == (const form_ref_lightweight& left, const FormRef& right) {
        return comp::equal(left, right);
    }

    template<class FormRef>
    inline bool operator == (const form_ref& left, const FormRef& right) {
        return comp::equal(left, right);
    }

    template<class FormRef>
    inline bool operator != (const form_ref_lightweight& left, const FormRef& right) {
        return !comp::equal(left, right);
    }

    template<class FormRef>
    inline bool operator != (const form_ref& left, const FormRef& right) {
        return !comp::equal(left, right);
    }

    template<class FormRef>
    inline bool operator < (const form_ref_lightweight& left, const FormRef& right) {
        return comp::less(left, right);
    }

    template<class FormRef>
    inline bool operator < (const form_ref& left, const FormRef& right) {
        return comp::less(left, right);
    }

}

namespace collections {

    using forms::form_ref;
    using forms::form_ref_lightweight;
    using forms::FormId;

    template<class Context>
    inline form_ref make_weak_form_id(FormId id, Context& context) {
        return form_ref{ id, context._form_watcher };
    }

    template<class Context>
    inline form_ref make_weak_form_id(const TESForm* form, Context& context) {
        return form ? form_ref(*form, context._form_watcher) : form_ref();
    }

    template<class Context>
    inline form_ref_lightweight make_lightweight_form_ref(FormId id, Context& context) {
        return form_ref_lightweight{ id, context._form_watcher };
    }

    template<class Context>
    inline form_ref_lightweight make_lightweight_form_ref(const TESForm* form, Context& context) {
        return form_ref_lightweight{ form ? util::to_enum<FormId>(form->formID) : FormId::Zero, context._form_watcher };
    }

}

namespace std {
    template<> inline void swap(forms::form_ref& left, forms::form_ref& right) {
        left.swap(right);
    }
}

BOOST_CLASS_VERSION(forms::form_observer, 3);
