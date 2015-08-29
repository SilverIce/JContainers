#pragma once

#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/algorithm/string/join.hpp>
#include <assert.h>
#include <inttypes.h>

#include "skse/GameForms.h"
#include "skse/PapyrusVM.h"
#include "skse/skse.h"

#include "collections/form_handling.h"
#include "collections/dyn_form_watcher.h"

namespace collections {

    namespace form_watching {

        namespace fh = form_handling;

        template<class ...Params>
        inline void log(const char* fmt, Params&& ...ps) {
            std::string s("FormWatching: ");
            s += fmt;
            skse::console_print(s.c_str(), std::forward<Params>(ps) ...);
        }

        class watched_form : public boost::noncopyable {

            const FormId _handle = FormId::Zero;
            std::atomic<bool> _deleted = false;

        public:

            watched_form() = delete;

            explicit watched_form(FormId handle)
                : _handle(handle)
            {
                log("watched_form retains %X", handle);
                skse::retain_handle(handle);
            }

            ~watched_form() {
                if (false == is_deleted()) {
                    log("watched_form releases %X", _handle);
                    skse::release_handle(_handle);
                }
            }

            bool is_deleted() const {
                return _deleted.load(std::memory_order_acquire);
            }

            void set_deleted() {
                _deleted.store(true, std::memory_order_release);
            }
        };

        dyn_form_watcher dyn_form_watcher::_instance;

        dyn_form_watcher::dyn_form_watcher() {
            _is_inside_unsafe_func._My_flag = false;
        }

        void dyn_form_watcher::on_form_deleted(FormHandle handle)
        {
            // already failed, there are plenty of any kind of objects that are deleted every moment, even during initial splash screen
            //jc_assert_msg(form_handling::is_static((FormId)handle) == false,
                //"If failed, then there is static form destruction event too? fId %" PRIX64, handle);

            if (!fh::is_form_handle(handle)) {
                //log("on_form_deleted: skipped %" PRIX64, handle);
                return;
            }

            // to test whether static form gets ever destroyed or not
            //jc_assert(form_handling::is_static((FormId)handle) == false);

            log("on_form_deleted: %" PRIX64, handle);

            auto formId = fh::form_handle_to_id(handle);

            if_condition_then_perform(
                [formId](const dyn_form_watcher& w) {
                    return w._watched_forms.find(formId) != w._watched_forms.end();
                },
                [formId](dyn_form_watcher& w) {
                    auto itr = w._watched_forms.find(formId);
                    if (itr != w._watched_forms.end()) {
                        auto watched = itr->second.lock();
                        if (watched) {
                            watched->set_deleted();
                            log("flag handle %" PRIX64 " as deleted", formId);
                        }
                        w._watched_forms.erase(itr);
                    }
                },
                *this
            );
        }

        boost::shared_ptr<watched_form> dyn_form_watcher::watch_form(FormId fId)
        {
            write_lock l{ _mutex };
            return u_watch_form(fId);
        }

        struct lock_or_fail {
            std::atomic_flag& flag;

            lock_or_fail(std::atomic_flag& flg) : flag(flg) {
                jc_assert_msg(false == flg.test_and_set(std::memory_order_acquire),
                    "My dyn_form_watcher test has failed? Report this please");
            }

            ~lock_or_fail() {
                flag.clear(std::memory_order_release);
            }
        };

        boost::shared_ptr<watched_form> dyn_form_watcher::u_watch_form(FormId fId)
        {
            log("watching form %X", fId);

            lock_or_fail g{ _is_inside_unsafe_func };

            auto itr = _watched_forms.find(fId);
            if (itr != _watched_forms.end() && !itr->second.expired()) {
                return itr->second.lock();
            }
            else {
                auto watched = boost::make_shared<watched_form>(fId);
                _watched_forms[fId] = watched;
                return watched;
            }
        }

        void tiny_test(FormId id) {
/*
            auto form = skse::lookup_form(id);
            if (form) {
                auto handle = (*g_objectHandlePolicy)->Create(TESForm::kTypeID, form);
                jc_assert(form_handling::is_form_handle(FormHandle(handle)));
            }*/
        }

        weak_form_id::weak_form_id(FormId id)
            : _id(id)
            , _expired(skse::lookup_form(id) == nullptr)
        {
            if (!fh::is_static(id) && !_expired) {
                _watched_form = dyn_form_watcher::instance().watch_form(id);
            }

            tiny_test(id);
        }

        weak_form_id::weak_form_id(const TESForm& form)
            : _id(static_cast<FormId>(form.formID))
            , _expired(false)
        {
            if (!fh::is_static(_id)) {
                _watched_form = dyn_form_watcher::instance().watch_form(_id);
            }

            tiny_test(_id);
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
                    log("weak_form_id: form %X known as deleted now", _id);
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