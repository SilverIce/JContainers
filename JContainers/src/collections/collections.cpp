

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>
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
#include "util/stl_ext.h"

#include "intrusive_ptr.hpp"
#include "intrusive_ptr_serialization.hpp"
#include "util/istring_serialization.h"
#include "iarchive_with_blob.h"

#include "object/object_base_serialization.h"

#include "forms/form_handling.h"

#include "collections/collections.h"
#include "collections/context.h"

#include "collections/context.hpp"
#include "forms/form_observer.hpp"

BOOST_CLASS_EXPORT_GUID(collections::array, "kJArray");
BOOST_CLASS_EXPORT_GUID(collections::map, "kJMap");
BOOST_CLASS_EXPORT_GUID(collections::form_map, "kJFormMap");
BOOST_CLASS_EXPORT_GUID(collections::integer_map, "kJIntegerMap");

BOOST_CLASS_VERSION(collections::form_map, 1)
BOOST_CLASS_VERSION(collections::item, 3)

BOOST_CLASS_IMPLEMENTATION(boost::blank, boost::serialization::primitive_type);

namespace collections {

    template<class Archive>
    struct converter_324_to_330 : public boost::static_visitor < > {
        template<class T> void operator () ( T& v) {
            var = std::move(v);
        }
        void operator () ( FormId& v) {
            auto& fwatcher = hack::iarchive_with_blob::from_base_get<tes_context>(archive)._form_watcher;
            var = form_ref{ v, fwatcher, form_ref::load_old_id };
        }
        item::variant& var;
        Archive& archive;

        explicit converter_324_to_330(item::variant& var_, Archive& archive_) : var(var_), archive(archive_) {}
    };
    
    
    template<class Archive>
    void item::load(Archive & ar, const unsigned int version)
    {
        switch (version)
        {
        default:
            BOOST_ASSERT(false);
            break;
        case 2: { // v 3.2.X and below
            using variant_old = boost::variant<boost::blank, SInt32, Real, FormId, internal_object_ref, std::string>;
            variant_old var;
            ar >> var;
            var.apply_visitor(converter_324_to_330<Archive>{ _var, ar });
        }
            break;
        case 3:
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
    void form_map::save(Archive & ar, const unsigned int version) const {
        ar & boost::serialization::base_object<object_base>(*this);
        ar & cnt;
    }

    template<class Archive>
    void form_map::load(Archive & ar, const unsigned int version) {
        ar & boost::serialization::base_object<object_base>(*this);

        switch (version) {
        default:
            BOOST_ASSERT_MSG(false, "invalid form_map version");
            break;
        case 0: {   // v3.2.X -> v3.3.X
            std::map<FormId, item> oldMap;
            ar >> oldMap;
            auto& fwatcher = hack::iarchive_with_blob::from_base_get<tes_context>(ar)._form_watcher;
            for (auto& pair : oldMap) {
                form_ref key{ pair.first, fwatcher, form_ref::load_old_id };
                if (key) {
                    cnt.emplace(value_type{ std::move(key), std::move(pair.second) });
                }
            }
        }
            break;
        case 1:
            ar & cnt;
            break;
        }
    }

    template<class Archive>
    void integer_map::serialize(Archive & ar, const unsigned int version) {
        ar & boost::serialization::base_object<object_base>(*this);
        ar & cnt;
    }

    //////////////////////////////////////////////////////////////////////////

    void form_map::u_onLoaded() {

        util::tree_erase_if(cnt, [](const value_type& pair){
            return pair.first.is_expired();
        });
    }

    //////////////////////////////////////////////////////////////////////////

    void array::u_nullifyObjects() {
        for (auto& item : _array) {
            item.u_nullifyObject();
        }
    }

    //////////////////////////////////////////////////////////////////////////
}
