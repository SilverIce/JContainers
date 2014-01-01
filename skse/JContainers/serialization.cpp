#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/split_free.hpp>

#include "collections.h"

namespace boost { namespace serialization {

    using namespace collections;

/*
    template<class Archive>
    inline void serialize(Archive & ar, CString & s, const unsigned int file_version) {
        split_free(ar, s, file_version); 
    }*/


    template<class Archive>
    void serialize(Archive & ar, collection_registry& reg, const unsigned int version) {
        ar.register_type(static_cast<array *>(NULL));
        ar.register_type(static_cast<map *>(NULL));

        ar & reg._map;
        ar & reg._idGen;
    }


}
} 
