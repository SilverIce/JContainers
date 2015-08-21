#pragma once

#include <hash_map>
//#include <memory>
#include <atomic>
#include "boost/shared_ptr.hpp"

#include "types.h"
#include "rw_mutex.h"
#include "skse/skse.h"
#include "skse/PapyrusVM.h"
#include "boost/serialization/split_member.hpp"
//#include "intrusive_ptr.hpp"
#include "collections/form_handling.h"

namespace collections {
    
namespace form_watching {

    namespace fh = form_handling;

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

    public:

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

        void on_form_deleted(FormId fId) {
            _DMESSAGE("dyn_form_watcher: flag form 0x%x as deleted", fId);

            if_condition_then_perform(
                [fId](const dyn_form_watcher& w) {
                    return w._watched_forms.find(fId) != w._watched_forms.end();
                },
                [fId](dyn_form_watcher& w) {
                    auto itr = w._watched_forms.find(fId);
                    if (itr != w._watched_forms.end()) {
                        //skse::console_print("dyn_form_watcher:on_form_deleted form %x", fId);

                        itr->second->set_deleted();
                        w._watched_forms.erase(itr);
                    }
                },
                *this
            );
        }

        void u_clearState() {
            _watched_forms.clear();
        }

        /*void u_remove_unwatched_forms() {
            for (auto itr = _watched_forms.begin(); itr != _watched_forms.end(); ) {
                if (itr->second.use_count() == 1) {
                    itr = _watched_forms.erase(itr);
                }
                else {
                    ++itr;
                }
            }
        }*/

        boost::shared_ptr<watched_form> watch_form(FormId fId) {
            write_lock l{ _mutex };
            return u_watch_form(fId);
        }

        boost::shared_ptr<watched_form> u_watch_form(FormId fId) {
            //skse::console_print("dyn_form_watcher:watch_form form %x", fId);
            _DMESSAGE("dyn_form_watcher: form 0x%x being watched", fId);

            auto itr = _watched_forms.find(fId);
            if (itr != _watched_forms.end()) {
                return itr->second;
            }
            else {

                return _watched_forms.emplace(
                    watched_forms_t::value_type{
                        fId,
                        boost::shared_ptr < watched_form > {new watched_form{}}
                    }
                ).first->second;
            }
        }

/*
        template<class Archive> void serialize(Archive & ar, const unsigned int version) {
            ar & _watched_forms;
        }
*/

    private:

        void unwatch_form(FormId fId) {
            write_lock l{ this->_mutex };
            this->_watched_forms.erase(fId);
        }
    };

    class weak_form_id {

        FormId _id = FormZero;
        mutable boost::shared_ptr<watched_form> _watched_form;
        mutable bool _expired = true;

    public:

        weak_form_id() {}

        explicit weak_form_id(FormId id)
            : _id(id)
            , _expired(skse::lookup_form(id) == nullptr)
        {
            if (!fh::is_static(id) && !_expired) {
                _watched_form = dyn_form_watcher::instance().watch_form(id);
            }
        }

        explicit weak_form_id(const TESForm& form)
            : _id(static_cast<FormId>(form.formID))
            , _expired(false)
        {
            if (!fh::is_static(_id)) {
                _watched_form = dyn_form_watcher::instance().watch_form(_id);
            }
        }

        bool is_not_expired() const {
            if (fh::is_static(_id)) {
                return !_expired;
            }

            if (_watched_form) {
                if (!_watched_form->is_deleted()) {
                    return true;
                }
                else {
                    _DMESSAGE("weak_form_id: form 0x%x lazily zeroed out", _id);
                    _watched_form = nullptr;
                    _expired = true;
                }
            }

            return false;
        }

        FormId get() const {
            return is_not_expired() ? _id : FormZero;
        }

/*
        void set(FormId id) {
            _id = id;

            if (!fh::is_static(_id)) {
                _watched_form = dyn_form_watcher::instance().watch_form(id);
            } else {
                _watched_form = nullptr;
            }
        }

*/
/*
        weak_form_id& operator = (FormId id) {
            set(id);
            return *this;
        }*/

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

        template<class Archive> void save(Archive & ar, const unsigned int version) const {
            bool expired = !is_not_expired();

            ar & _id;
            ar & expired;
        }

        template<class Archive> void load(Archive & ar, const unsigned int version) {
            ar & const_cast<FormId&>(_id);
            ar & _expired;

            if (!_expired) {
                if (form_handling::is_static(_id)) {
                    _id = (FormId)skse::resolve_handle((uint32_t)_id);
                    _expired = (_id == FormZero);
                }
                else { // otherwise it is dynamic form

                    if (skse::lookup_form((uint32_t)_id)) {
                        _watched_form = dyn_form_watcher::instance().u_watch_form(_id);
                    }
                    else {
                        _expired = true;
                    }
                }
            }
        }

    };

}

    using form_watching::weak_form_id;

    template<class Context>
    inline weak_form_id make_weak_form_id(FormId id, Context& context) {
        return weak_form_id(id, context.form_watcher);
    }

    template<class Context>
    inline weak_form_id make_weak_form_id(TESForm* id, Context& context) {
        return weak_form_id(id ? FormId(id->formID) : FormZero, context.form_watcher);
    }

}
