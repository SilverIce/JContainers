#pragma once

#include <boost/serialization/split_free.hpp>
#include <boost/serialization/nvp.hpp>
#include <atomic>
#include "boost/serialization/level.hpp"
#include "boost/serialization/tracking.hpp"

namespace boost {
    namespace serialization {

        template<class Archive, class T>
        void save(Archive & ar, const std::atomic<T> & v, const unsigned int version) {
            T value = v.load(std::memory_order_relaxed);
            ar << BOOST_SERIALIZATION_NVP(value);
        }

        template<class Archive, class T>
        void load(Archive & ar, std::atomic<T> & v, const unsigned int version) {
            T value = 0;
            ar >> BOOST_SERIALIZATION_NVP(value);
            v.store(value, std::memory_order_relaxed);
        }

        template<class Archive, class T>
        inline void serialize(
            Archive & ar,
            std::atomic<T> & v,
            const unsigned int file_version
            ){
            split_free(ar, v, file_version);
        }

        // BOOST_CLASS_IMPLEMENTATION
        template <class T>
        struct implementation_level_impl< const std::atomic<T> >
        {
            typedef mpl::integral_c_tag tag;
            typedef mpl::int_< object_serializable > type;
            BOOST_STATIC_CONSTANT(
                int,
                value = implementation_level_impl::type::value
            );
        };
    }
}
