#pragma once

#include "boost\serialization\split_free.hpp"
#include "boost\serialization\version.hpp"
#include "boost\serialization\optional.hpp"

#include "object_base.h"

namespace boost { namespace serialization {

    namespace cl = collections;

    template<class Archive, class T>
    void save_atomic(Archive& ar, const std::atomic<T>& v) {
        T refCnt = v.load(std::memory_order_relaxed);
        ar & refCnt;
    }

    template<class Archive, class T>
    void load_atomic (Archive& ar, std::atomic<T> & v) {
        T refCnt = 0;
        ar & refCnt;
        v.store(refCnt, std::memory_order_relaxed);
    }

    template<class Archive>
    void save(Archive & ar, const cl::object_base & t, unsigned int version) {
        //jc_assert(version == 1);
        jc_assert(t._stack_refCount.load(std::memory_order_relaxed) == 0);
        jc_assert(t.noOwners() == false);

        save_atomic(ar, t._refCount);
        save_atomic(ar, t._tes_refCount);
        ar << t._id;
        ar << t._tag;
    }

    template<class Archive>
    void load(Archive & ar, cl::object_base & t, unsigned int version) {
        load_atomic(ar, t._refCount);
        load_atomic(ar, t._tes_refCount);

        switch (version) {
        case 1:
            ar >> t._id;
            ar >> t._tag;
            break;
        case 0:
            ar >> t._type;
            ar >> t._id;
            break;
        default:
            jc_assert(false);
            break;
        }

        // trying detect objects with no owners
        jc_assert(version == 0 || t.noOwners() == false);
    }

}
}

BOOST_SERIALIZATION_SPLIT_FREE(collections::object_base);
BOOST_CLASS_VERSION(collections::object_base, 1);
