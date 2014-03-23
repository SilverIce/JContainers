

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include <boost/serialization/split_member.hpp>
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

#include "tes_context.hpp"

extern SKSESerializationInterface	* g_serialization;

BOOST_CLASS_EXPORT_GUID(collections::array, "kJArray");
BOOST_CLASS_EXPORT_GUID(collections::map, "kJMap");
BOOST_CLASS_EXPORT_GUID(collections::form_map, "kJFormMap");

namespace collections {

#ifndef TEST_COMPILATION_DISABLED

    TEST(Item, serialization)
    {
        std::ostringstream str;
        ::boost::archive::binary_oarchive arch(str);

        const char *testStr = "hey, babe!";

        auto ar = array::create();
        ar->u_push(Item(testStr));

        {
            arch << Item(3);
            arch << Item(3.5);
            arch << Item(testStr);

            map obj;
            obj["tttt"] = Item(testStr);
            obj["array"] = Item(ar);

            arch << obj;
        }
        ar->release();

        // reading
        std::string string = str.str();
        std::istringstream istr(string);
        boost::archive::binary_iarchive ia(istr);

        Item item;
        ia >> item;
        EXPECT_TRUE(item.intValue() == 3);
        ia >> item;
        EXPECT_TRUE(item.fltValue() == 3.5);

        ia >> item;
        EXPECT_TRUE(strcmp(item.strValue(),testStr) == 0);

        map obj;
        ia >> obj;

        EXPECT_TRUE(obj.u_count() == 2);
        //EXPECT_TRUE(strcmp(obj["array"].strValue(), testStr) == 0);
    }

    template<class T>
    struct Hum {
        T val;

        explicit Hum(T value) : val(value) {}

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & val;
        }
    };

    TEST(Item, serialization2)
    {
        std::ostringstream str;
        ::boost::archive::binary_oarchive arch(str);

        std::map<UInt32, Item> oldStruct;
        oldStruct[0xbaadc0de] = Item("testt");
        oldStruct[0xf0f0] = Item("ololo");

        arch << oldStruct;
       /* arch << Hum<UInt32>(999);*/

        // reading
        std::string string = str.str();
        std::istringstream istr(string);
        boost::archive::binary_iarchive ia(istr);

        std::map<FormId, Item, std::greater<FormId> > newStruct;
        ia >> newStruct;

/*
        Hum<UInt64> val(0);
        ia >> val;
*/

        EXPECT_TRUE(oldStruct.size() == newStruct.size());
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

/*
    TEST_DISABLED(shit, shit)
    {
        {
            std::ostringstream str;
            ::boost::archive::binary_oarchive arch(str);
            arch << Item(3);
        }


        std::vector<char> buffer;
        auto inserter = boost::iostreams::back_inserter(buffer);

        boost::iostreams::stream< decltype(inserter) > s(inserter);
        boost::archive::binary_oarchive arch(s);

        //s->write("hello", 5);
        //EXPECT_TRUE(s == false);

        Item itm("empty");
        arch << itm;

        EXPECT_TRUE(buffer.empty() == false);
    }*/

#endif

    static UInt32 convertOldFormIdToNew(UInt32 oldId) {
        UInt64 newId = 0;
        g_serialization->ResolveHandle(oldId, &newId);
        return newId;
    }

    template<class Archive>
    static UInt32 readOldFormIdToNew(Archive& ar) {
        UInt32 oldId = 0;
        ar & oldId;
        return convertOldFormIdToNew(oldId);
    }

    template<class Archive>
    void Item::save(Archive & ar, const unsigned int version) const {
        ar & _type;
        switch (_type)
        {
        case ItemTypeNone:
            break;
        case ItemTypeInt32:
            ar & _intVal;
            break;
        case ItemTypeFloat32:
            ar & _floatVal;
            break;
        case ItemTypeCString: 
            ar & std::string(strValue() ? _stringVal->string : "");
            break;
        case ItemTypeObject:
            ar & _object;
            break;
        case ItemTypeForm:
            ar & _formId;
            break;
        default:
            break;
        }
    }

    template<class Archive>
    void Item::load(Archive & ar, const unsigned int version)
    {
        ar & _type;
        switch (_type)
        {
        case ItemTypeNone:
            break;
        case ItemTypeInt32:
            ar & _intVal;
            break;
        case ItemTypeFloat32:
            ar & _floatVal;
            break;
        case ItemTypeCString:
            {
                std::string string;
                ar & string;

                if (!string.empty()) {
                    _stringVal = StringMem::allocWithString(string.c_str());
                }
                break;
            }
        case ItemTypeObject:
            ar & _object;
            break;
        case ItemTypeForm:
            _formId = readOldFormIdToNew(ar);
            break;
        default:
            break;
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
    if (intfc->OpenRecord(kJStorageChunk, kJSerializationDataVersion)) {
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
