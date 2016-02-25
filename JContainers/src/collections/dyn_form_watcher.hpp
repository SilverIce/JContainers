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
#include "util/util.h"

#include "collections/form_handling.h"
#include "collections/dyn_form_watcher.h"

BOOST_CLASS_VERSION(collections::form_watching::form_ref, 2);
BOOST_CLASS_VERSION(collections::form_watching::form_observer, 3);

namespace collections {

    namespace form_watching {

        namespace fh = form_handling;

        template<class ...Params>
        inline void log(const char* fmt, Params&& ...ps) {
            JC_log(fmt, std::forward<Params>(ps) ...);
        }

        class form_entry : public boost::noncopyable {

            FormId _handle = FormId::Zero;
            std::atomic<bool> _deleted = false;
            // to not release the handle if the handle wasn't be previously retained (for ex. handle's object was not loaded)
            bool _is_handle_retained = false;

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
                if (!u_is_deleted() && _is_handle_retained) {
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

            bool u_is_deleted() const {
                return _deleted._My_val;
            }
            void u_set_deleted() {
                _deleted._My_val = true;
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
/*
            util::tree_erase_if(_watched_forms, [](const watched_forms_t::value_type& pair) {
                return pair.second.expired();
            });*/
        }

        void form_observer::u_print_status() const
        {
            uint32_t count_of_one_user = 0;
            uint32_t dyn_form_count = 0;

            for (auto& pair : _watched_forms) {
                if (!pair.second.expired()) {
                    log("%" PRIX32 " : %u", pair.first, pair.second.use_count());
                    if (pair.second.use_count() == 1) {
                        ++count_of_one_user;
                    }
                    if (!fh::is_static(pair.first)) {
                        ++dyn_form_count;
                    }
                }
            }

            log("total %u", _watched_forms.size());
            log("count_of_one_user %u", count_of_one_user);
            log("dyn_form_count %u", dyn_form_count);

        }

        namespace {

            static boost::detail::spinlock & spinlock_for(FormId formId) {
                using spinlock_pool = boost::detail::spinlock_pool < 'DyFW' > ;
                return spinlock_pool::spinlock_for(reinterpret_cast<void*>(formId));
            }
        }

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
                auto itr = _watched_forms.find(formId);
                if (itr != _watched_forms.end()) {
                    // read and write

                    std::lock_guard<boost::detail::spinlock> guard{ spinlock_for(formId) };
                    auto watched = itr->second.lock();

                    if (watched) {
                        watched->set_deleted();
                        log("flag form-entry %" PRIX32 " as deleted", formId);
                    }
                    itr->second.reset();
                }
            }
        }

        template<class Archive, class Collection, class ElementSaver>
        void save_collection(Archive& archive, const Collection& collection, ElementSaver&& saver) {
            uint32_t count = collection.size();
            archive << count;
            for (const auto& pair : collection) {
                saver(archive, pair);
            }
        }
        template<class Archive, class Collection, class ElementLoader>
        void load_collection(Archive& archive, Collection& collection, ElementLoader&& loader) {
            uint32_t count = 0;
            archive >> count;

            while (count > 0) {
                --count;
                loader(archive, collection);
            }
        }

        template<class Archive>
        void form_observer::load(Archive & ar, const unsigned int version) {

            switch (version) {
            case 3:
                load_collection(ar, _watched_forms, [&ar](Archive& ar, decltype(_watched_forms)& collection) {
                    form_entry_ref entry;
                    ar >> entry;

                    if (entry && !entry->is_deleted()) {
                        collection[entry->id()] = std::move(entry);
                    }
                });
                break;
            case 2:{
                std::hash_map<FormId, boost::weak_ptr<form_entry> > oldCnt;
                ar >> oldCnt;

                for (auto& pair : oldCnt) {
                    form_entry_ref entry = pair.second.lock();
                    if (entry && !entry->is_deleted()) {
                        _watched_forms[entry->id()] = std::move(entry);
                    }
                }
            }
                break;
            default:
                jc_assert_msg(false, "Older versions of form_observer shouldn't be in release builds. In first place");
                break;
            }
        }

        template<class Archive>
        void form_observer::save(Archive & ar, const unsigned int version) const {

            save_collection(ar, _watched_forms, [&ar](Archive& ar, const decltype(_watched_forms)::value_type& pair) {
                auto entry = pair.second.lock();
                ar << entry;
            });
        }

        form_entry_ref form_observer::watch_form(FormId fId)
        {
            if (fId == FormId::Zero) {
                return nullptr;
            }

            log("watching form %X", fId);

            {
                std::lock_guard<boost::detail::spinlock> guard{ spinlock_for(fId) };

                auto itr = _watched_forms.find(fId);
                if (itr != _watched_forms.end()) {
                    auto watched = itr->second.lock();
                    if (!watched) {
                        // what if two threads trying assign??
                        // both threads are here or one is here and another performing @on_form_deleted func.
                        itr->second = watched = form_entry::make(fId);
                    }
                    return watched;
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

        template<class Archive>
        void form_ref::save(Archive & ar, const unsigned int version) const
        {
            // optimization: if the form was deleted (is_not_expired is false) - write null instead

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
                    auto& watcher = hack::iarchive_with_blob::from_base_get<tes_context>(ar)._form_watcher;
                    _watched_form = watcher.watch_form(id);
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
                    auto& watcher = hack::iarchive_with_blob::from_base_get<tes_context>(ar)._form_watcher;
                    _watched_form = watcher.watch_form(id);
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

            TEST(form_entry_ref, _)
            {
                form_entry e;

                e.set_deleted();
                e.is_deleted();
            }

            TEST(form_watching, perft){

                form_observer watcher;
                util::do_with_timing("form_observer performance", [&](){


                    for (int i = 0; i < 1000000; ++i) {
                        watcher.watch_form(util::to_enum<FormId>(i % 1000));
                    }

                });
            }


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

/*
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
            }*/

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

            // 
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

                std::map<form_ref, int> forms = { { form_ref{ fid, watcher }, 0 } };
                watcher.on_form_deleted(fhid);

                EXPECT_TRUE(forms.begin()->first.is_expired());

                forms[form_ref{ fid, watcher }] = 0;

                EXPECT_TRUE(forms.size() == 2);
                // one of the keys should be non-expired
                EXPECT_TRUE(forms.begin()->first.is_not_expired() != (++forms.begin())->first.is_not_expired());
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