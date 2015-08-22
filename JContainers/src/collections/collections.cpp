

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/hash_map.hpp>

#include <boost/serialization/split_member.hpp>
#include <boost/serialization/split_free.hpp>

#include <boost/serialization/version.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <fstream>
#include <sstream>
#include <set>

#include "gtest.h"

#include "intrusive_ptr.hpp"
#include "intrusive_ptr_serialization.hpp"
#include "util/istring_serialization.h"

#include "object/object_base_serialization.h"

#include "collections/collections.h"
#include "collections/context.h"
#include "collections/form_handling.h"

#include "collections/context.hpp"
#include "collections/dyn_form_watcher_serialization.hpp"

BOOST_CLASS_EXPORT_GUID(collections::array, "kJArray");
BOOST_CLASS_EXPORT_GUID(collections::map, "kJMap");
BOOST_CLASS_EXPORT_GUID(collections::form_map, "kJFormMap");
BOOST_CLASS_EXPORT_GUID(collections::integer_map, "kJIntegerMap");

BOOST_CLASS_VERSION(collections::item, 2)

BOOST_CLASS_IMPLEMENTATION(boost::blank, boost::serialization::primitive_type);

namespace collections {
    
    template<class Archive>
    void item::load(Archive & ar, const unsigned int version)
    {
        switch (version)
        {
        default:
            BOOST_ASSERT(false);
            break;
        case 2:
            ar & _var;
            break;
        }
    }

    template<class Archive>
    void item::save(Archive & ar, const unsigned int version) const {
        ar & _var;
    }

    //////////////////////////////////////////////////////////////////////////

    template<class Archive>
    void array::serialize(Archive & ar, const unsigned int version) {
        ar & boost::serialization::base_object<object_base>(*this);
        ar & _array;
    }

    template<class Archive>
    void map::serialize(Archive & ar, const unsigned int version) {
        ar & boost::serialization::base_object<object_base>(*this);
        ar & cnt;
    }

    template<class Archive>
    void form_map::serialize(Archive & ar, const unsigned int version) {
        ar & boost::serialization::base_object<object_base>(*this);
        ar & cnt;
    }

    template<class Archive>
    void integer_map::serialize(Archive & ar, const unsigned int version) {
        ar & boost::serialization::base_object<object_base>(*this);
        ar & cnt;
    }

    //////////////////////////////////////////////////////////////////////////

    void form_map::u_onLoaded() {
        cnt.erase(weak_form_id{});
    }

    //////////////////////////////////////////////////////////////////////////

    void array::u_nullifyObjects() {
        for (auto& item : _array) {
            item.u_nullifyObject();
        }
    }

    //////////////////////////////////////////////////////////////////////////
}
