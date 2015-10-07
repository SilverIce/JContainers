#pragma once

#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/range.hpp>
//#include <boost/algorithm/string/join.hpp>
#include <assert.h>
#include <inttypes.h>
#include <map>
#include <tuple>
#include <mutex>

//#include "skse/GameForms.h"
//#include "skse/PapyrusVM.h"
#include "skse/skse.h"
#include "util/stl_ext.h"

#include "collections/form_handling.h"
#include "collections/dyn_form_watcher.h"

BOOST_CLASS_VERSION(collections::form_watching::form_ref, 2);

namespace collections {

    namespace form_watching {

        namespace fh = form_handling;

        template<class ...Params>
        inline void log(const char* fmt, Params&& ...ps) {
            JC_log(fmt, std::forward<Params ...>(ps ...));
        }

        class form_entry : public boost::noncopyable {

            FormId _handle = FormId::Zero;
            std::atomic<bool> _deleted = false;
            // to not release the handle if the handle can't be retained (for ex. handle's object was not loaded)
            bool _is_handle_retained = false;

            bool u_is_deleted() const {
                return _deleted._My_val;
            }
            void u_set_deleted() {
                _deleted._My_val = true;
            }

        public:

            form_entry(FormId handle, bool deleted, bool handle_was_retained)
                : _handle(handle)
                , _deleted(deleted)
                , _is_handle_retained(handle_was_retained)
            {}

            form_entry() = default;

            static form_entry_ref make(FormId handle) {
                //log("form_entry retains %X", handle);

                return boost::make_shared<form_entry>(
                    handle,
                    false,
                    skse::try_retain_handle(handle));
            }

            static form_entry_ref make_expired(FormId handle) {
                return boost::make_shared<form_entry>(handle, true, false);
            }

            ~form_entry() {
                if (!is_deleted() && _is_handle_retained) {
                    //log("form_entry releases %X", _handle);
                    skse::release_handle(_handle);
                }
            }

            FormId id() const { return _handle; }

            bool is_deleted() const {
                return _deleted.load(std::memory_order_acquire);
            }

            void set_deleted() {
                _deleted.store(true, std::memory_order_release);
            }

            friend class boost::serialization::access;
            BOOST_SERIALIZATION_SPLIT_MEMBER();

            template<class Archive> void save(Archive & ar, const unsigned int version) const {
                ar << util::to_integral_ref(_handle);
                ar << _deleted._My_val;
            }

            template<class Archive> void load(Archive & ar, const unsigned int version) {
                ar >> util::to_integral_ref(_handle);
                ar >> _deleted._My_val;

                if (u_is_deleted() == false) {
                    _handle = skse::resolve_handle(_handle);

                    if (_handle != FormId::Zero) {
                        _is_handle_retained = skse::try_retain_handle(_handle);
                    }
                    else {
                        u_set_deleted();
                    }
                }
            }
        };

        void form_observer::u_remove_expired_forms() {
            util::tree_erase_if(_watched_forms, [](const watched_forms_t::value_type& pair) {
                return pair.second.expired();
            });
        }

        form_observer::form_observer() {
            _is_inside_unsafe_func._My_flag = false;
        }

        using spinlock_pool = boost::detail::spinlock_pool < 'DyFW' > ;

        void form_observer::on_form_deleted(FormHandle handle)
        {
            // already failed, there are plenty of any kind of objects that are deleted every moment, even during initial splash screen
            //jc_assert_msg(form_handling::is_static((FormId)handle) == false,
                //"If failed, then there is static form destruction event too? fId %" PRIX64, handle);

            if (!fh::is_form_handle(handle)) {
                return;
            }

            // to test whether static form gets ever destroyed or not
            //jc_assert(form_handling::is_static((FormId)handle) == false);

            ///log("on_form_deleted: %" PRIX64, handle);

            auto formId = fh::form_handle_to_id(handle);
            {
                read_lock r(_mutex);

                auto itr = _watched_forms.find(formId);
                if (itr != _watched_forms.end()) {
                    // read and write

                    std::lock_guard<boost::detail::spinlock> guard{ spinlock_pool::spinlock_for(&itr->second) };
                    auto watched = itr->second.lock();

                    if (watched) {
                        watched->set_deleted();
                        log("flag form-entry %" PRIX32 " as deleted", formId);
                    }
                    itr->second.reset();
                }
            }
        }

        form_entry_ref form_observer::watch_form(FormId fId)
        {
            if (fId == FormId::Zero) {
                return nullptr;
            }

            log("watching form %X", fId);

            auto get_or_assign = [fId](boost::weak_ptr<form_entry> & watched_weak) -> form_entry_ref {
                std::lock_guard<boost::detail::spinlock> guard{ spinlock_pool::spinlock_for(&watched_weak) };
                auto watched = watched_weak.lock();

                if (!watched) {
                    // what if two threads trying assign??
                    // both threads are here or one is here and another performing @on_form_deleted func.
                    watched_weak = watched = form_entry::make(fId);
                }
                return watched;
            };

            {
                read_lock r(_mutex);
                auto itr = _watched_forms.find(fId);

                if (itr != _watched_forms.end()) {
                    return get_or_assign(itr->second);
                }
            }

            {
                write_lock r(_mutex);

                auto itr = _watched_forms.find(fId);

                if (itr != _watched_forms.end()) {
                    return get_or_assign(itr->second);
                }
                else {
                    auto watched = form_entry::make(fId);
                    _watched_forms[fId] = watched;
                    return watched;
                }
            }
        }

        struct lock_or_fail {
            std::atomic_flag& flag;

            explicit lock_or_fail(std::atomic_flag& flg) : flag(flg) {
                jc_assert_msg(false == flg.test_and_set(std::memory_order_acquire),
                    "My dyn_form_watcher test has failed? Report this please");
            }

            ~lock_or_fail() {
                flag.clear(std::memory_order_release);
            }
        };

        ////////////////////////////////////////

        form_ref::form_ref(FormId id, form_observer& watcher)
            : _watched_form(watcher.watch_form(id))
        {
        }

        form_ref::form_ref(const TESForm& form, form_observer& watcher)
            : _watched_form(watcher.watch_form(util::to_enum<FormId>(form.formID)))
        {
        }

        bool form_ref::is_not_expired() const
        {
            return _watched_form && !_watched_form->is_deleted();
        }

        form_ref::form_ref(FormId oldId, form_observer& watcher, load_old_id_t)
            : _watched_form(watcher.watch_form(skse::resolve_handle(oldId)))
        {
        }

        form_ref form_ref::make_expired(FormId formId) {
            auto entry = form_entry::make_expired(formId);
            return form_ref{ entry };
        }


        //////////////////

        FormId form_ref::get() const {
            return is_not_expired() ? _watched_form->id() : FormId::Zero;
        }

        FormId form_ref::get_raw() const {
            return _watched_form ? _watched_form->id() : FormId::Zero;
        }

        bool form_ref::operator < (const form_ref& o) const {
            return _watched_form < o._watched_form;

            // Form Ids are not reliable, sorting is not stable, not persistent
            /*auto getIdent = [](const watched_form * wf) {
                return wf ? wf->identifier() : 0;
            };

            return getIdent(_watched_form.get()) < getIdent(o._watched_form.get());*/
        }

        template<class Archive>
        void form_ref::save(Archive & ar, const unsigned int version) const
        {
            // optimization: the form was deleted - write null instead

            if (is_not_expired())
                ar << _watched_form;
            else {
                decltype(_watched_form) fake;
                ar << fake;
            }
        }

        template<class Archive>
        void form_ref::load(Archive & ar, const unsigned int version)
        {

            switch (version)
            {
            case 0: {// v3.3 alpha-1 format
                FormId oldId = FormId::Zero;
                ar >> oldId;
                FormId id = skse::resolve_handle(oldId);
                bool expired = false;
                ar >> expired;

                if (!expired) {
                    auto watcher = hack::iarchive_with_blob::from_base_get<tes_context>(ar).form_watcher.get();
                    _watched_form = watcher->watch_form(id);
                }
                break;
            }
            case 1: {
                // Remove this case !!! This format wasn't ever published
                FormId oldId = FormId::Zero;
                ar >> oldId;
                FormId id = skse::resolve_handle(oldId);
                bool expired = false;
                ar >> expired;

                if (!expired) {
                    ar >> _watched_form;
                }
                else {
                    auto watcher = hack::iarchive_with_blob::from_base_get<tes_context>(ar).form_watcher.get();
                    _watched_form = watcher->watch_form(id);
                }
                break;
            }
            case 2:
                ar >> _watched_form;
                break;
            default:
                assert(false);
                break;
            }
        }

        namespace tests {

            namespace bs = boost;

            TEST(form_watching, simple){
                form_ref id;

                EXPECT_TRUE(!id);
                EXPECT_TRUE(id.get() == FormId::Zero);
                EXPECT_TRUE(id.get_raw() == FormId::Zero);
            }

            TEST(form_watching, simple_2){
                const auto fid = util::to_enum<FormId>(0xff000014);
                form_observer watcher;
                form_ref id{ fid, watcher };

                EXPECT_FALSE(!id);
                EXPECT_TRUE(id.get() == fid);
                EXPECT_TRUE(id.get_raw() == fid);

                {
                    form_ref copy = id;
                    EXPECT_FALSE(!copy);
                    EXPECT_TRUE(copy.get() == fid);
                    EXPECT_TRUE(copy.get_raw() == fid);
                }
            }

            TEST(form_observer, u_remove_expired_forms){
                form_observer watcher;

                const auto fid = util::to_enum<FormId>(0xff000014);

                auto entry = watcher.watch_form(fid);
                EXPECT_TRUE(watcher.u_forms_count() == 1);
                EXPECT_NOT_NIL(entry.get());

                watcher.on_form_deleted(fh::form_id_to_handle(fid));
                watcher.u_remove_expired_forms();

                EXPECT_TRUE(watcher.u_forms_count() == 0);
                EXPECT_TRUE(entry->is_deleted());
            }

            TEST(form_watching, bug_1)
            {
                const auto fid = util::to_enum<FormId>(0x14);
                form_observer watcher;
                form_ref non_expired{ fid, watcher };

                std::vector<form_ref> forms = { form_ref::make_expired(fid) };

                EXPECT_FALSE(std::find(forms.begin(), forms.end(), non_expired) != forms.end()); // had to be EXPECT_FALSE
            }

            template<class T, class V>
            bool contains(T&& cnt, V&& value) {
                return cnt.find(value) != cnt.end();
            }

            TEST(form_watching, bug_2)
            {
                const auto fid = util::to_enum<FormId>(0x14);
                form_observer watcher;
                form_ref non_expired{ fid, watcher };

                std::map<form_ref, int> forms = { { form_ref::make_expired(fid), 0 } };

                EXPECT_FALSE( contains(forms, non_expired) ); // had to be EXPECT_FALSE
            }

            TEST(form_watching, bug_3)
            {
                const auto fid = util::to_enum<FormId>(0x14);
                form_observer watcher;
                form_ref non_expired{ fid, watcher };
                auto expired = form_ref::make_expired(fid);

                std::map<form_ref, int> forms = { { non_expired, 0 } };

                EXPECT_FALSE(contains(forms, expired)); // had to be EXPECT_FALSE
            }

            TEST(form_watching, bug_4)
            {
                const auto fid = util::to_enum<FormId>(0xff000014);
                const auto fhid = fh::form_id_to_handle(fid);

                form_observer watcher;
                form_ref id{ fid, watcher };

                std::map<form_ref, int> forms = { { id, 0 } };

                watcher.on_form_deleted(fhid);

                forms.insert({ form_ref{ fid, watcher }, 0 });

                EXPECT_TRUE(forms.size() == 2);

                watcher.on_form_deleted(fhid);
                // @forms contains equal keys now!

                EXPECT_TRUE(forms.size() == 2);

            }

            TEST(form_watching, bug_5)
            {
                const auto fid = util::to_enum<FormId>(0xff000014);
                const auto fhid = fh::form_id_to_handle(fid);

                form_observer watcher;

                std::map<form_ref, int> forms = { { form_ref{ fid, watcher }, 0 } };
                watcher.on_form_deleted(fhid);

                EXPECT_TRUE(forms.begin()->first.is_expired());

                forms[form_ref{ fid, watcher }] = 0;

                EXPECT_TRUE(forms.begin()->first.is_not_expired());
            }

            TEST(form_watching, dynamic_form_id){
                const auto fid = util::to_enum<FormId>(0xff000014);
                const auto fhid = fh::form_id_to_handle(fid);
               // EXPECT_TRUE(fh::is_static(fid) == false);

                form_observer watcher;
                form_ref id{ fid, watcher };
                form_ref id2{ fid, watcher };

                auto expectNotExpired = [&](const form_ref& id) {
                    EXPECT_TRUE(id.is_not_expired());
                    EXPECT_TRUE(id.get() == fid);
                };

                auto expectExpired = [&](const form_ref& id) {
                    EXPECT_FALSE(id.is_not_expired());
                    EXPECT_TRUE(id.get() == FormId::Zero);
                    EXPECT_TRUE(id.get_raw() == fid);
                };

                expectNotExpired(id);
                expectNotExpired(id2);

                watcher.on_form_deleted(fhid);

                expectExpired(id);
                expectExpired(id2);
            }
        }

    }
}