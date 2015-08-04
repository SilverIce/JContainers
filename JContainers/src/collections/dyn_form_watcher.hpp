#pragma once

#include "skse/GameForms.h"
#include "skse/skse.h"
#include "collections/form_handling.h"
#include "collections/dyn_form_watcher.h"

namespace collections {

    namespace form_watching {

        namespace fh = form_handling;

        void dyn_form_watcher::on_form_deleted(FormId fId)
        {
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

        boost::shared_ptr<watched_form> dyn_form_watcher::watch_form(FormId fId)
        {
            _DMESSAGE("dyn_form_watcher: form 0x%x being watched", fId);

            write_lock l{ _mutex };
            return u_watch_form(fId);
        }

        struct lock_or_fail {
            std::atomic_flag& flag;

            lock_or_fail(std::atomic_flag& flg) : flag(flg) {
                BOOST_ASSERT_MSG(false == flg.test_and_set(std::memory_order_acquire),
                    "my dyn_form_watcher test has failed?");
            }

            ~lock_or_fail() {
                flag.clear(std::memory_order_release);
            }
        };

        boost::shared_ptr<watched_form> dyn_form_watcher::u_watch_form(FormId fId)
        {
            lock_or_fail g(_is_inside_unsafe_func);

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

        weak_form_id::weak_form_id(FormId id)
            : _id(id)
            , _expired(skse::lookup_form(id) == nullptr)
        {
            if (!fh::is_static(id) && !_expired) {
                _watched_form = dyn_form_watcher::instance().watch_form(id);
            }
        }

        weak_form_id::weak_form_id(const TESForm& form)
            : _id(static_cast<FormId>(form.formID))
            , _expired(false)
        {
            if (!fh::is_static(_id)) {
                _watched_form = dyn_form_watcher::instance().watch_form(_id);
            }
        }

        bool weak_form_id::is_not_expired() const
        {
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

        template<class Archive>
        void weak_form_id::save(Archive & ar, const unsigned int version) const
        {
            bool expired = !is_not_expired();
            ar << _id << expired;
        }

        template<class Archive>
        void weak_form_id::load(Archive & ar, const unsigned int version)
        {
            auto id = FormId::Zero;
            bool expired = true;

            ar >> id >> expired;

            if (!expired) {
                u_load_old_form_id(id);
            }
            else {
                _id = id;
                _expired = expired;
            }
        }

        void weak_form_id::u_load_old_form_id(FormId oldId)
        {
            if (form_handling::is_static(oldId)) {
                FormId newId = skse::resolve_handle(oldId);
                if (newId != FormId::Zero) {
                    _id = newId;
                    _expired = false;
                }
                else {
                    _id = oldId;
                    _expired = true;
                }
            }
            else { // otherwise it is dynamic form
                if (skse::lookup_form(oldId)) {
                    _watched_form = dyn_form_watcher::instance().u_watch_form(oldId);
                    _expired = false;
                }
                else {
                    _expired = true;
                }

                _id = oldId;
            }
        }

    }
}