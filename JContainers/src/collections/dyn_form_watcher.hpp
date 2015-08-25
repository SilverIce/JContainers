#pragma once

#include "boost/smart_ptr/make_shared_object.hpp"
#include <boost/algorithm/string/join.hpp>

#include "skse/GameForms.h"
#include "skse/PapyrusVM.h"
#include "skse/skse.h"

#include "collections/form_handling.h"
#include "collections/dyn_form_watcher.h"

namespace collections {

    namespace form_watching {

        dyn_form_watcher dyn_form_watcher::_instance;

        namespace fh = form_handling;

        template<class ...Params>
        inline void log(const char* fmt, Params&& ...ps) {
            std::string s("FormWatching: ");
            s += fmt;
            skse::console_print(s.c_str(), std::forward<Params>(ps) ...);
        }

        void dyn_form_watcher::on_form_deleted(FormHandle fId)
        {
            BOOST_ASSERT_MSG(form_handling::is_static((FormId)fId) == false,
                "If failed, then there is static form destruction event too?");

            if_condition_then_perform(
                [fId](const dyn_form_watcher& w) {
                    return w._watched_forms.find(fId) != w._watched_forms.end();
                },
                [fId](dyn_form_watcher& w) {
                    auto itr = w._watched_forms.find(fId);
                    if (itr != w._watched_forms.end()) {
                        auto watched = itr->second.lock();
                        if (watched) {
                            watched->set_deleted();
                            log("flag form %x as deleted", fId);
                        }
                        w._watched_forms.erase(itr);
                    }
                },
                *this
            );
        }

        boost::shared_ptr<watched_form> dyn_form_watcher::watch_form(FormId fId)
        {
            log("form %x being watched", fId);

            write_lock l{ _mutex };
            return u_watch_form(fId);
        }

        struct lock_or_fail {
            std::atomic_flag& flag;

            lock_or_fail(std::atomic_flag& flg) : flag(flg) {
                BOOST_ASSERT_MSG(false == flg.test_and_set(std::memory_order_acquire),
                    "My dyn_form_watcher test has failed? Report this please");
            }

            ~lock_or_fail() {
                flag.clear(std::memory_order_release);
            }
        };

        boost::shared_ptr<watched_form> dyn_form_watcher::u_watch_form(FormId fId)
        {
            log("watching form %x", fId);

            lock_or_fail g{ _is_inside_unsafe_func };

            FormHandle handle = form_handling::form_id_to_handle(fId);

            auto itr = _watched_forms.find(handle);
            if (itr != _watched_forms.end() && !itr->second.expired()) {
                return itr->second.lock();
            }
            else {
                auto watched = boost::make_shared<watched_form>(fId);
                _watched_forms[handle] = watched;
                return watched;
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
                    log("weak_form_id: form %x is known as deleted now", _id);
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