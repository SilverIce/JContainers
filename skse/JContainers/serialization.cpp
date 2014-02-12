#include <boost/serialization/serialization.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

#include <fstream>
#include <sstream>
#include <set>

#include "gtest.h"

#include "collections.h"
#include "autorelease_queue.h"
#include "shared_state.h"

#include "shared_state.hpp"

BOOST_CLASS_VERSION(collections::object_base, 1);

namespace collections {

#ifndef TEST_COMPILATION_DISABLED

    TEST(Item, serialization)
    {
        std::ostringstream str;
        ::boost::archive::binary_oarchive arch(str);

        const char *testStr = "hey, babe!";

        auto ar = new array;
        ar->push(Item(testStr));

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

        EXPECT_TRUE(obj.cnt.size() == 2);
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

        arch << oldStruct;

        // reading
        std::string string = str.str();
        std::istringstream istr(string);
        boost::archive::binary_iarchive ia(istr);

        std::map<FormId, Item> newStruct;
        ia >> newStruct;

        EXPECT_TRUE(oldStruct.begin()->first == newStruct.begin()->first);
    }

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
    }

#endif

/*
    template<class Archive>
    inline void serialize(Archive & ar, CString & s, const unsigned int file_version) {
        split_free(ar, s, file_version); 
    }*/

    template<class T> void registerContainers(T & ar) {
        ar.register_type(static_cast<array *>(NULL));
        ar.register_type(static_cast<map *>(NULL));
        ar.register_type(static_cast<form_map *>(NULL));
    }

    template<class Archive>
    void collection_registry::serialize(Archive & ar, const unsigned int version) {
        registerContainers(ar);

        ar & _map;
        ar & _idGen;
    }

    template<class Archive>
    void Item::save(Archive & ar, const unsigned int version) const {
        registerContainers(ar);

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
            ar & _uintVal;
            break;
        default:
            break;
        }
    }

    template<class Archive>
    void Item::load(Archive & ar, const unsigned int version)
    {
        registerContainers(ar);

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
            ar & _uintVal;
            break;
        default:
            break;
        }
    }

    template<class Archive>
    void object_base::serialize(Archive & ar, const unsigned int version) {
        ar & _refCount;
        ar & _tes_refCount;
        
        ar & _type;
        ar & _id;
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

    template<class Archive>
    void form_map::serialize(Archive & ar, const unsigned int version) {
        ar & boost::serialization::base_object<object_base>(*this);
        ar & cnt;
    }
}

