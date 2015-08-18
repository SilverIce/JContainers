#pragma once

#include <hash_map>
#include <memory>

#include "types.h"
#include "rw_mutex.h"
#include "form_handling.h"
//#include "intrusive_ptr.hpp"

namespace collections { namespace form_watching {

    class dyn_form_watcher;

    class watched_form {
    public:

        bool _deleted = false;

        void set_deleted() {
            _deleted = true;
        }
    };

    // - remove FormID if none watches it
    // - 

    class dyn_form_watcher {

        using watched_forms_t = std::hash_map<FormId, std::shared_ptr<watched_form> >;

        bshared_mutex _mutex;
        watched_forms_t _watched_forms;

    public:

        void on_form_deleted(FormId fId) {
            write_lock l(_mutex);
            auto itr = _watched_forms.find(fId);
            if (itr != _watched_forms.end()) {
                itr->second->set_deleted();
                _watched_forms.erase(itr);
            }
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

        std::shared_ptr<watched_form> watch_form(FormId fId) {
            write_lock l(_mutex);
            auto itr = _watched_forms.find(fId);
            if (itr != _watched_forms.end()) {
                return itr->second;
            }
            else {
                std::shared_ptr<watched_form> wForm{
                    new watched_form()/*,
                    [this, fId](watched_form* p) {
                        this->unwatch_form(fId);
                    }*/
                };

                _watched_forms.emplace(watched_forms_t::value_type(fId, wForm));
                return wForm;
            }
        }

        void unwatch_form(FormId fId) {
            write_lock l(this->_mutex);
            this->_watched_forms.erase(fId);

        }
    };

    class weak_form_id {
    public:

        FormId _id = FormZero;
        mutable std::shared_ptr<watched_form> _watched_form;

        weak_form_id() {}

        explicit weak_form_id(FormId id, dyn_form_watcher& watcher) {
            set(id, watcher);
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

        void set(FormId id, dyn_form_watcher& watcher) {
            _id = id;

            if (!form_handling::is_static(_id)) {
                _watched_form = watcher.watch_form(id);
            }
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

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & _id;
            assert(false);
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
