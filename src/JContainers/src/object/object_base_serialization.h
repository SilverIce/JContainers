#pragma once

#include "boost\serialization\split_free.hpp"
#include "boost\serialization\version.hpp"
#include "boost\serialization\optional.hpp"

#include "util/atomic_serialization.h"
#include "object_base.h"

namespace boost { namespace serialization {

    namespace cl = collections;

    template<class Archive>
    void save(Archive & ar, const cl::object_base & t, unsigned int version) {
        //jc_assert(version == 1);

        // Lua retains an objects with _stack_refCount counter. Asertion disabled 
        //jc_assert(t._stack_refCount.load(std::memory_order_relaxed) == 0);
        jc_assert(t.noOwners() == false);

        switch (version) {
        case 2:
            ar << t._aqueue_push_time;
            break;
        case 1:
            save_atomic(ar, t._refCount); // may not store it in v2.0 anymore
            break;
        case 0:
        default:
            jc_assert(false);
            break;
        }

        save_atomic(ar, t._tes_refCount);
        save_atomic(ar, t._id);
        ar << *reinterpret_cast<std::string const*> (&t._tag); //force Boost detection
    }

    template<class Archive>
    void load(Archive & ar, cl::object_base & t, unsigned int version) {
        switch (version) {
        case 0:
        case 1: {
            std::atomic_int32_t unused_refCount;
            load_atomic(ar, unused_refCount);
            break;
        }
        case 2:
            ar >> t._aqueue_push_time;
            break;
        }

        load_atomic(ar, t._tes_refCount);

        switch (version) {
        case 2:
        case 1:
            load_atomic (ar, t._id);
            ar >> *reinterpret_cast<std::string*> (&t._tag); //force Boost detection
            break;
        case 0:
            ar >> t._type;
            load_atomic(ar, t._id);
            break;
        default:
            jc_assert(false);
            break;
        }

        // "trying detect objects with no owners" - not possible to do this assertion anymore:
        // Lua retains an objects with _stack_refCount counter. Asertion disabled 
        //jc_assert(version == 0 || t.noOwners() == false);
    }

}
}

BOOST_SERIALIZATION_SPLIT_FREE(collections::object_base);
BOOST_CLASS_VERSION(collections::object_base, 2);
