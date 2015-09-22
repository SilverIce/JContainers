#pragma once

#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/range.hpp>
//#include <boost/algorithm/string/join.hpp>
#include <assert.h>
#include <inttypes.h>
#include <map>
#include <tuple>

//#include "skse/GameForms.h"
//#include "skse/PapyrusVM.h"
#include "skse/skse.h"
#include "util/stl_ext.h"

#include "collections/form_handling.h"
#include "collections/dyn_form_watcher.h"

BOOST_CLASS_VERSION(collections::form_watching::weak_form_id, 1);

namespace collections {

    namespace form_watching {

        namespace fh = form_handling;

        void log(const char* fmt, ...) {
            va_list	args;
            va_start(args, fmt);
            JC_log(fmt, args);
            va_end(args);
        }

        class watched_form : public boost::noncopyable {

            FormId _handle = FormId::Zero;
            std::atomic<bool> _deleted = false;

        public:

            watched_form() = default;

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

            friend class boost::serialization::access;
            BOOST_SERIALIZATION_SPLIT_MEMBER();

            template<class Archive> void save(Archive & ar, const unsigned int version) const {
                ar & reinterpret_cast<const std::underlying_type<decltype(_handle)>::type &>(_handle);
                ar & _deleted._My_val;
            }

            template<class Archive> void load(Archive & ar, const unsigned int version) {
                ar & reinterpret_cast<std::underlying_type<decltype(_handle)>::type &>(_handle);
                ar & _deleted._My_val;

                // retain handle again
                if (is_deleted() == false) {
                    skse::retain_handle(_handle);
                }
            }
        };

        void dyn_form_watcher::remove_expired_forms() {
            util::tree_erase_if(_watched_forms, [](watched_forms_t::value_type& pair) {
                return pair.second.expired();
            });
        }

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

            ///log("on_form_deleted: %" PRIX64, handle);

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
            jc_assert_msg(fh::is_static(fId) == false, "we don't watch static forms (yet?)");

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

        weak_form_id::weak_form_id(FormId id, dyn_form_watcher& watcher)
            : _id(id)
            , _expired(skse::lookup_form(id) == nullptr)
        {
            if (!fh::is_static(id) && !_expired) {
                _watched_form = watcher.watch_form(id);
            }
        }

        weak_form_id::weak_form_id(const TESForm& form, dyn_form_watcher& watcher)
            : _id(static_cast<FormId>(form.formID))
            , _expired(false)
        {
            if (!fh::is_static(_id)) {
                _watched_form = watcher.watch_form(_id);
            }
        }

        bool weak_form_id::is_not_expired() const
        {
            if (fh::is_static(_id)) {
                return !_expired;
            }

            return _watched_form && !_watched_form->is_deleted();
        }

        weak_form_id::weak_form_id(FormId oldId, dyn_form_watcher& watcher, load_old_id_t) {
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
                _watched_form = watcher.u_watch_form(oldId);
                _expired = false;
                _id = oldId;
            }
        }

        /*
         I'll just hope mod authors who use JContainers follow the news

         It seems I'll be forced to erase invalid form-keys from JFormMap containers, otherwise they may be filled up with lot of died forms and bring even more issues..
         and lot of ambiguity.

         Say, a JFormMap contains some form-key, which points to deleted, non-persistent form identifier - 0xff000014,
         Skyrim creates new form and reuses 0xff000014  identifier, and a user performs JFormMap.hasKey test on that container. It had to returns False, as the keys are different

         JFormMap.nextKey is another pray as the function may return None (due to non-persistent forms), so iteration process will be interrupted.
         Thery is really no way, expect to not use @nextKey on JFormMap (use @allKeys or @allKeyAsPArray instead), unless you sure it will never contain non-persistent forms

         As I said there is lot of ambiguity. What if I'll not remove ghost-forms (deleted forms and forms from disabled plugins)?

         - arrays will still contain ghost-forms (deleted forms), as always, as before
         - form-maps may suffer due to plenty of ghost-forms. JFormMap.nextKey will have more faults than now
         - JFormDB will likey grow continuously due to plenty of ghost-forms

         ======
         As I said, it some kind of Pandora Box and it's not because of form retaining. The vein of 'lets preserve everything' is not that easily achievable.

         Lot of questions raised. First of all, is ghost-form equality. Say there is 0xff000014 form in JArray container, the form gets deleted, Skyrim creates new form, 
         reuses 0xff000014 identifier, a user inserts the form in the array, (and optionally - the form gets deleted).
         Should JArray.unique left only one form? Are these forms are equal or not? Probably not, though from outer point of view both forms are 'None'.

         Repeat the same as above for JFormMap and its keys. Now some important information associated with deleted form-key and in v3.3 I'm trying to not lost that information, make it accessible.
         JFormMap is {deleted-form: data}, Skyrim creates @newForm, reuses 0xff000014 identifier. Should JFormMap.hasKey(newForm) return true? No. Thus this time the forms aren't equal.
        */

        bool weak_form_id::operator < (const weak_form_id& o) const {
            return std::make_tuple(_id, _expired, _watched_form) < std::make_tuple(o._id, o._expired, o._watched_form);
            //return _id < o._id;
            //return std::make_tuple(_id, _watched_form) < std::make_tuple(o._id, o._watched_form);

            /*
                Don't  USE  THIS!!
            if (_id < o._id) {
                return true;
            }
            else if (_id > o._id) {
                return false;
            }

            return is_not_expired() < o.is_not_expired();*/
        }

        template<class Archive>
        void weak_form_id::save(Archive & ar, const unsigned int version) const
        {
            bool expired = is_expired();

            ar << _id << expired;

            if (!expired) {
                ar << _watched_form;
            }
        }

        template<class Archive>
        void weak_form_id::load(Archive & ar, const unsigned int version)
        {
            auto oldId = FormId::Zero;
            bool expired = true;

            ar >> oldId >> expired;

            switch (version)
            {
            case 0: {
                // v3.3 alpha-1 format. retrieve dyn_form_watcher instance
                // it's not good code, obviuously
                auto watcher = tes_context::instance().form_watcher.get();
                if (!expired && skse::lookup_form(oldId)) {
                    _watched_form = watcher->u_watch_form(oldId);
                }
                else {
                    expired = true;
                }
                break;
            }
            case 1:
                if (!expired) {
                    ar >> _watched_form;
                }
                break;
            default:
                assert(false);
                break;
            }

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
            } else {
                _expired = expired;
                _id = oldId;
            }
        }

        namespace tests {

            namespace bs = boost;

            TEST(form_watching, simple){
                weak_form_id id;

                EXPECT_TRUE(!id);
                EXPECT_TRUE(id.get() == FormId::Zero);
                EXPECT_TRUE(id.get_raw() == FormId::Zero);
            }

            TEST(form_watching, simple_2){
                const auto fid = (FormId)0xff000014;
                dyn_form_watcher watcher;
                weak_form_id id{ fid, watcher };

                EXPECT_FALSE(!id);
                EXPECT_TRUE(id.get() == fid);
                EXPECT_TRUE(id.get_raw() == fid);

                {
                    weak_form_id copy = id;
                    EXPECT_FALSE(!copy);
                    EXPECT_TRUE(copy.get() == fid);
                    EXPECT_TRUE(copy.get_raw() == fid);
                }
            }

            TEST(form_watching, bug_1)
            {
                const auto fid = (FormId)0x14;
                dyn_form_watcher watcher;
                weak_form_id non_expired{ fid, watcher };

                std::vector<weak_form_id> forms = { weak_form_id::make_expired(fid) };

                EXPECT_FALSE(std::find(forms.begin(), forms.end(), non_expired) != forms.end()); // had to be EXPECT_FALSE
            }

            template<class T, class V>
            bool contains(T&& cnt, V&& value) {
                return cnt.find(value) != cnt.end();
            }

            TEST(form_watching, bug_2)
            {
                const auto fid = (FormId)0x14;
                dyn_form_watcher watcher;
                weak_form_id non_expired{ fid, watcher };

                std::map<weak_form_id, int> forms = { { weak_form_id::make_expired(fid), 0 } };

                EXPECT_FALSE( contains(forms, non_expired) ); // had to be EXPECT_FALSE
            }

            TEST(form_watching, bug_3)
            {
                const auto fid = (FormId)0x14;
                dyn_form_watcher watcher;
                weak_form_id non_expired{ fid, watcher };
                auto expired = weak_form_id::make_expired(fid);

                std::map<weak_form_id, int> forms = { { non_expired, 0 } };

                EXPECT_FALSE(contains(forms, expired)); // had to be EXPECT_FALSE
            }

            TEST(form_watching, bug_4)
            {
                const auto fid = (FormId)0xff000014;
                const auto fhid = fh::form_id_to_handle(fid);

                dyn_form_watcher watcher;
                weak_form_id id{ fid, watcher };

                std::map<weak_form_id, int> forms = { { id, 0 } };

                watcher.on_form_deleted(fhid);

                forms.insert({ weak_form_id{ fid, watcher }, 0 });

                EXPECT_TRUE(forms.size() == 2);

                watcher.on_form_deleted(fhid);
                // @forms contains equal keys now!

                EXPECT_TRUE(forms.size() == 2);

            }

            TEST(form_watching, bug_5)
            {
                const auto fid = (FormId)0xff000014;
                const auto fhid = fh::form_id_to_handle(fid);

                dyn_form_watcher watcher;

                std::map<weak_form_id, int> forms = { { weak_form_id{ fid, watcher }, 0 } };
                watcher.on_form_deleted(fhid);

                EXPECT_TRUE(forms.begin()->first.is_expired());

                forms[weak_form_id{ fid, watcher }] = 0;

                EXPECT_TRUE(forms.begin()->first.is_not_expired());
            }

            TEST(form_watching, dynamic_form_id){
                const auto fid = (FormId)0xff000014;
                const auto fhid = fh::form_id_to_handle(fid);
               // EXPECT_TRUE(fh::is_static(fid) == false);

                dyn_form_watcher watcher;
                weak_form_id id{ fid, watcher };
                weak_form_id id2{ fid, watcher };

                auto expectNotExpired = [&](const weak_form_id& id) {
                    EXPECT_TRUE(id.is_not_expired());
                    EXPECT_TRUE(id.get() == fid);
                };

                auto expectExpired = [&](const weak_form_id& id) {
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