#pragma once

#include "boost/serialization/split_free.hpp"

#include "util/atomic_serialization.h"
#include "collections/dyn_form_watcher.h"

namespace boost {
namespace serialization {

    namespace fw = collections::form_watching;

    template<class Archive>
    inline void save(Archive & ar, const fw::watched_form & t, unsigned int version) {
        save_atomic(ar, t._deleted);
    }
    template<class Archive>
    inline void load(Archive & ar, fw::watched_form & t, unsigned int version) {
        load_atomic(ar, t._deleted);
    }

}
}

BOOST_SERIALIZATION_SPLIT_FREE(collections::form_watching::watched_form);

