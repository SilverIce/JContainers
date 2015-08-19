#pragma once

#include <hash_map>
#include <memory>
#include "boost/shared_ptr.hpp"

#include "types.h"
#include "rw_mutex.h"
#include "collections/form_handling.h"
#include "skse/skse.h"
//#include "intrusive_ptr.hpp"

namespace collections { namespace form_watching {

    class dyn_form_watcher;

    class watched_form {
    public:

        bool _deleted = false;

        void set_deleted() {
            _deleted = true;
        }

        template<class Archive> void serialize(Archive & ar, const unsigned int version) {
            ar & _deleted;
        }
    };

    // - remove FormID if none watches it
    // - 

    // Had to be single instance as single Skyrim instance only?
    class dyn_form_watcher {

        using watched_forms_t = std::hash_map<FormId, boost::shared_ptr<watched_form> >;

        bshared_mutex _mutex;
        watched_forms_t _watched_forms;

    public:

        static dyn_form_watcher& instance() {
            static dyn_form_watcher _inst;
            return _inst;
        }

        void on_form_deleted(FormId fId) {
            write_lock l(_mutex);
            auto itr = _watched_forms.find(fId);
            if (itr != _watched_forms.end()) {
                skse::console_print("dyn_form_watcher:on_form_deleted form %x", fId);
                itr->second->set_deleted();
                _watched_forms.erase(itr);
            }
        }

        void u_clearState() {
            _watched_forms.clear();
        }

        void u_remove_unwatched_forms() {
            for (auto itr = _watched_forms.begin(); itr != _watched_forms.end(); ) {
                if (itr->second.use_count() == 1) {
                    itr = _watched_forms.erase(itr);
                }
                else {
                    ++itr;
                }
            }
        }

        boost::shared_ptr<watched_form> watch_form(FormId fId) {
            write_lock l(_mutex);
            return u_watch_form(fId);
        }

        boost::shared_ptr<watched_form> u_watch_form(FormId fId) {
            skse::console_print("dyn_form_watcher:watch_form form %x", fId);

            auto itr = _watched_forms.find(fId);
            if (itr != _watched_forms.end()) {
                return itr->second;
            }
            else {
                boost::shared_ptr<watched_form> wForm{
                    new watched_form()/*,
                    [this, fId](watched_form* p) {
                        this->unwatch_form(fId);
                    }*/
                };

                _watched_forms.emplace(watched_forms_t::value_type(fId, wForm));
                return wForm;
            }
        }

        template<class Archive> void serialize(Archive & ar, const unsigned int version) {
            ar & _watched_forms;
        }

    private:

        void unwatch_form(FormId fId) {
            write_lock l(this->_mutex);
            this->_watched_forms.erase(fId);
        }
    };

    class weak_form_id {

        FormId _id = FormZero;
        mutable boost::shared_ptr<watched_form> _watched_form;

    public:

        weak_form_id() {}

        // move constructors:

        explicit weak_form_id(FormId id) {
            set(id);
        }

        bool expired() const {
            return !_form_exists();
        }

        bool _form_exists() const {
            if (_watched_form) {
                if (!_watched_form->_deleted) {
                    return true;
                }
                else {
                    _watched_form = nullptr;
                    skse::console_print("weak_form_id:_form_exists form %x died", _id);
                }
            }

            return false;
        }

        FormId get() const {
            if (form_handling::is_static(_id)) {
                return _id;
            }

            return _form_exists() ? _id : FormZero;
        }

        void set(FormId id) {
            _id = id;

            if (!form_handling::is_static(_id)) {
                _watched_form = dyn_form_watcher::instance().watch_form(id);
            }
        }

        weak_form_id& operator = (FormId id) {
            set(id);
            return *this;
        }

        bool operator == (const weak_form_id& o) const {
            return _id == o._id;
        }
        bool operator != (const weak_form_id& o) const {
            return !(*this == o);
        }
        bool operator < (const weak_form_id& o) const {
            return _id < o._id;
        }

        template<class Archive> void serialize(Archive & ar, const unsigned int version) {
            ar & _id;

/*
            if (!form_handling::is_static(_id)) {
                _watched_form = 
            }*/

            ar & _watched_form;
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
