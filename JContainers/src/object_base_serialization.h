#pragma once

#include "boost\serialization\split_free.hpp"

#include "object_base.h"

namespace boost { namespace serialization {

    namespace cl = collections;

    template<class Archive>
    void save(Archive & ar, const cl::object_base & t, unsigned int version) {
        jc_assert(t._stack_refCount.load(std::memory_order_relaxed) == 0);
        int32_t refCnt = t._refCount.load(std::memory_order_relaxed);
        int32_t tesCnt = t._tes_refCount.load(std::memory_order_relaxed);

        ar & refCnt;
        ar & tesCnt;

        ar & t._type;
        ar & t._id;
    }

    template<class Archive>
    void load(Archive & ar, cl::object_base & t, unsigned int version) {
        int32_t refCnt = 0, tesCnt = 0;
        ar & refCnt;
        ar & tesCnt;
        ar & t._type;
        ar & t._id;

        t._refCount.store(refCnt, std::memory_order_relaxed);
        t._tes_refCount.store(tesCnt, std::memory_order_relaxed);
    }

}
}

BOOST_SERIALIZATION_SPLIT_FREE(collections::object_base);
