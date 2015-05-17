#pragma once

#include "intrusive_ptr.hpp"

#include <boost/serialization/split_free.hpp>
#include <boost/serialization/nvp.hpp>
#include "boost/serialization/level.hpp"
#include "boost/serialization/tracking.hpp"

namespace boost {
namespace serialization {

    template<class Archive, class T, class P>
    void save(Archive & ar, const intrusive_ptr_jc<T, P> & v, const unsigned int version) {
        T* value = v.get();
        ar << BOOST_SERIALIZATION_NVP(value);
    }

    template<class Archive, class T, class P>
    void load(Archive & ar, intrusive_ptr_jc<T, P> & v, const unsigned int version) {
        T* value = nullptr;
        ar >> BOOST_SERIALIZATION_NVP(value);
        v = intrusive_ptr_jc<T, P>(value); 
    }

    template<class Archive, class T, class P>
    inline void serialize(
        Archive & ar,
        intrusive_ptr_jc<T, P> & v,
        const unsigned int file_version
        ){
        split_free(ar, v, file_version);
    }
}
}

