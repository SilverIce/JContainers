

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>

#include <boost/serialization/split_member.hpp>
#include <boost/serialization/split_free.hpp>

#include <boost/serialization/version.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

//#include <boost/iostreams/device/array.hpp>
//#include <boost/iostreams/stream.hpp>
//#include <boost/iostreams/device/back_inserter.hpp>

#include <fstream>
#include <sstream>
#include <set>

#include "skse/PluginAPI.h"

#include "gtest.h"
#include "plugin_info.h"

#include "collections.h"
#include "tes_context.h"
#include "form_handling.h"

#include "tes_context.hpp"

extern SKSESerializationInterface	* g_serialization;

//BOOST_CLASS_EXPORT_GUID(collections::object_base, "kJValue");
BOOST_CLASS_EXPORT_GUID(collections::array, "kJArray");
BOOST_CLASS_EXPORT_GUID(collections::map, "kJMap");
BOOST_CLASS_EXPORT_GUID(collections::form_map, "kJFormMap");

BOOST_SERIALIZATION_SPLIT_FREE(collections::object_ref);

BOOST_CLASS_VERSION(collections::Item, 1)

namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive & ar, blank & g, const unsigned int version) {}

        template<class Archive>
        void save(Archive & ar, const collections::object_ref & ptr, const unsigned int version) {
            collections::object_base *obj = ptr.get();
            ar & obj;
        }

        template<class Archive>
        void load(Archive & ar, collections::object_ref & ptr, const unsigned int version) {
            collections::object_base *obj = nullptr;
            ar & obj;
            ptr = collections::object_ref(obj, false);
        }

    } // namespace serialization
} // namespace boost

namespace collections {

#ifndef TEST_COMPILATION_DISABLED

    TEST(Item, serialization)
    {
        std::ostringstream data;
        ::boost::archive::binary_oarchive arch(data);

        Item tst((const char*)nullptr);


        Item items[] = {
            Item(2.0),
            Item(2),
            Item("the most fatal mistake"),
            //Item(array::object()),
           // Item(map::object())
        };

        for (auto& itm : items) {
            arch << itm;
        }

        // reading
        std::string string = data.str();
        std::istringstream istr(string);
        boost::archive::binary_iarchive ia(istr);

        for (const auto& itm : items) {
            Item tmp;
            ia >> tmp;

            EXPECT_TRUE( itm.isEqual(tmp) );


        }
    }

/*
    TEST(autorelease_queue, serialization)
    {
        bshared_mutex mt;
        autorelease_queue queue(mt);
        //  queue.start();
        queue.push(10);
        queue.push(20);

        std::ostringstream str;
        ::boost::archive::binary_oarchive arch(str);
        arch << queue;

        bshared_mutex mt2;
        autorelease_queue queue2(mt2);
        // reading
        std::string string = str.str();
        std::istringstream istr(string);
        boost::archive::binary_iarchive ia(istr);

        ia >> queue2;

        EXPECT_TRUE(queue.count() == queue2.count());
        ;
    }*/

#endif

    static FormId convertOldFormIdToNew(FormId oldId) {
        if (form_handling::is_static(oldId)) {
            UInt64 newId = 0;
            g_serialization->ResolveHandle(oldId, &newId);
            return (FormId)newId;
        }
        else {
            return oldId;
        }
    }

    template<class Archive>
    static FormId readOldFormIdToNew(Archive& ar) {
        UInt32 oldId = 0;
        ar & oldId;
        return convertOldFormIdToNew((FormId)oldId);
    }

    template<class Archive>
    void Item::save(Archive & ar, const unsigned int version) const {
        ar & _var;
    }

    template<class Archive>
    void Item::load(Archive & ar, const unsigned int version)
    {
        if (version >= 1) {
            ar & _var;

            if (type() == ItemTypeForm) {
                *this = convertOldFormIdToNew(formId());
            }
        }
        else {

            ItemType type = ItemTypeNone;
            ar & type;

            switch (type)
            {
            case ItemTypeInt32: {
                SInt32 val = 0;
                ar & val;
                *this = val;
                break;
            }
            case ItemTypeFloat32: {
                Float32 val = 0;
                ar & val;
                *this = val;
                break;
            }
            case ItemTypeCString: {
                std::string string;
                ar & string;
                *this = string;
                break;
            }
            case ItemTypeObject: {
                object_base *object = nullptr;
                ar & object;
                _var = collections::object_ref(object, false);
                break;
            }
            case ItemTypeForm:
                *this = readOldFormIdToNew(ar);
                break;
            default:
                break;
            }
        }
    }

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

    //////////////////////////////////////////////////////////////////////////

    template<class Archive>
    void form_map::save(Archive & ar, const unsigned int version) const {
        ar & boost::serialization::base_object<object_base>(*this);
        ar & cnt;
    }

    template<class Archive>
    void form_map::load(Archive & ar, const unsigned int version) {
        ar & boost::serialization::base_object<object_base>(*this);
        ar & cnt;
    }

    void form_map::u_updateKeys() {

        std::vector<FormId> keys;
        keys.reserve(cnt.size());
        for (auto& pair : cnt) {
            keys.push_back(pair.first);
        }

        for(auto& oldKey : keys) {
            FormId newKey = static_cast<FormId>(convertOldFormIdToNew(oldKey));

            if (oldKey == newKey) {
                ;
            }
            else if (newKey == 0) {
                cnt.erase(oldKey);
            } else if (newKey != oldKey) {
                Item item = cnt[oldKey];
                cnt.erase(oldKey);
                cnt[newKey] = item;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void array::u_nullifyObjects() {
        for (auto& item : _array) {
            item.u_nullifyObject();
        }
    }

    void form_map::u_nullifyObjects() {
        for (auto& pair : cnt) {
            pair.second.u_nullifyObject();
        }
    }

    void map::u_nullifyObjects() {
        for (auto& pair : cnt) {
            pair.second.u_nullifyObject();
        }
    }
}

void Serialization_Revert(SKSESerializationInterface * intfc)
{
    collections::tes_context::instance().clearState();
}

void Serialization_Save(SKSESerializationInterface * intfc)
{
    if (intfc->OpenRecord(kJStorageChunk, kJSerializationCurrentVersion)) {
        auto data = collections::tes_context::instance().saveToArray();
        intfc->WriteRecordData(data.data(), data.size());
    }
}

void Serialization_Load(SKSESerializationInterface * intfc)
{
    UInt32	type;
    UInt32	version;
    UInt32	length;

    std::string data;

    while (intfc->GetNextRecordInfo(&type, &version, &length)) {

        if (type == kJStorageChunk && length > 0) {
            data.resize(length, '\0');
            intfc->ReadRecordData((void *)data.data(), data.size());
            break;
        }
    }

    collections::tes_context::instance().loadAll(data, version);
}
