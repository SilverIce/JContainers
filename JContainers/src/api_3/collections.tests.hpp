#pragma once

#include <future>
#include "util.h"

namespace tes_api_3 {

    using namespace collections;

    struct JCFixture : testing::Fixture {
        tes_context context;
    };

#   define JC_TEST(name, name2) TEST_F(JCFixture, name, name2)
#   define JC_TEST_DISABLED(name, name2) TEST_F(JCFixture, name, DISABLED_##name2)

}

#ifndef TEST_COMPILATION_DISABLED

// API-related tests:
namespace tes_api_3 {

    JC_TEST(array,  test)
    {
        array::ref arr = array::object(context);

        EXPECT_TRUE(tes_array::count(arr) == 0);

        tes_array::addItemAt<SInt32>(arr, 10);
        EXPECT_TRUE(tes_array::count(arr) == 1);
        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(arr, 0) == 10);

        tes_array::addItemAt<SInt32>( arr, 30);
        EXPECT_TRUE(tes_array::count(arr) == 2);
        EXPECT_TRUE(tes_array::itemAtIndex<SInt32>(arr, 1) == 30);

        array::ref arr2 = array::object(context);
        tes_array::addItemAt<SInt32>(arr2, 4);

        tes_array::addItemAt(arr, arr2.to_base<object_base>());
        EXPECT_TRUE(tes_array::itemAtIndex<object_base*>(arr, 2) == arr2.get());

        EXPECT_TRUE(tes_array::itemAtIndex<SInt32>( arr2, 0) == 4);
    }

    JC_TEST(array,  negative_indices)
    {
        array::ref obj = array::object(context);

        SInt32 values[] = {1,2,3,4,5,6,7};

        for (auto num : values) {
            tes_array::addItemAt(obj, num, -1);
        }

        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(obj, -1) == 7);
        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(obj, -2) == 6);

        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(obj, 0) == 1);
        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(obj, 1) == 2);

        tes_array::addItemAt(obj, 8, -2);
        //{1,2,3,4,5,6,8,7}
        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(obj, -2) == 8);

        tes_array::addItemAt(obj, 0, 0);
        //{0,1,2,3,4,5,6,8,7}
        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(obj, 0) == 0);

        tes_array::eraseIndex(obj, -1);
        //{0,1,2,3,4,5,6,8}
        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(obj, -1) == 8);
        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(obj, 7) == 8);

        tes_array::eraseIndex(obj, -2);
        //{0,1,2,3,4,5,8}
        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(obj, -2) == 5);

        tes_array::swapItems(obj, 0, -1);
        //{8,1,2,3,4,5,0}
        EXPECT_TRUE(tes_array::itemAtIndex<SInt32>(obj, 0) == 8 && tes_array::itemAtIndex<SInt32>(obj, -1) == 0);
    }

    TEST(array, sort_and_unique)
    {
        auto sort = [&](const char *jsonText) {
            auto& ar = tes_object::objectFromPrototype(jsonText)->as_link<array>();
            auto countBefore = ar.u_count();
            tes_array::sort(&ar);
            EXPECT_TRUE(countBefore == ar.u_count());
            EXPECT_TRUE(std::is_sorted(ar.u_container().begin(), ar.u_container().end()));
        };

        sort(STR([9, 8, 7, 20, 3]));
        sort(STR([1.0, "tempo", 0, "__formData||0x12"]));
        sort("[]");
    }

    TEST(tes_jcontainers, tes_jcontainers)
    {
        EXPECT_TRUE(tes_jcontainers::__isInstalled());

        EXPECT_FALSE(tes_jcontainers::fileExistsAtPath(nullptr));
        EXPECT_TRUE(!tes_jcontainers::fileExistsAtPath("abracadabra"));
    }

    TEST(path_resolving, collection_operators)
    {
        auto shouldReturnNumber = [&](object_base *obj, const char *path, float value) {
            path_resolving::resolve(tes_context::instance(), obj, path, [&](Item * item) {
                EXPECT_TRUE(item && item->fltValue() == value);
            });
        };

        auto shouldReturnInt = [&](object_base *obj, const char *path, int value) {
            path_resolving::resolve(tes_context::instance(), obj, path, [&](Item * item) {
                EXPECT_TRUE(item && item->intValue() == value);
            });
        };

        {
            object_base *obj = tes_object::objectFromPrototype(STR([1, 2, 3, 4, 5, 6]));

            shouldReturnNumber(obj, "@maxNum", 6);
            shouldReturnNumber(obj, "@minNum", 1);

            shouldReturnNumber(obj, "@minFlt", 0);
        }
        {
            object_base *obj = tes_object::objectFromPrototype(STR(
            { "a": 1, "b" : 2, "c" : 3, "d" : 4, "e" : 5, "f" : 6 }
            ));

            shouldReturnInt(obj, "@maxNum", 0);
            shouldReturnInt(obj, "@maxFlt", 0);

            shouldReturnInt(obj, "@maxNum.value", 6);
            shouldReturnInt(obj, "@minNum.value", 1);
        }
        {
            object_base *obj = tes_object::objectFromPrototype(STR(
            { "a": [1], "b" : {"k": -100}, "c" : [3], "d" : {"k": 100}, "e" : [5], "f" : [6] }
            ));

            shouldReturnInt(obj, "@maxNum", 0);
            shouldReturnInt(obj, "@maxFlt", 0);

            shouldReturnNumber(obj, "@maxNum.value[0]", 6);
            shouldReturnNumber(obj, "@minNum.value[0]", 1);

            shouldReturnNumber(obj, "@maxNum.value.k", 100);
            shouldReturnNumber(obj, "@minNum.value.k", -100);
        }
    }

    TEST(path_resolving, path_resolving)
    {

        object_base *obj = tes_object::objectFromPrototype(STR(
        {
            "glossary": {
                "GlossDiv": "S"
            },
            "array" : [["NPC Head [Head]", 0, -0.330000]],
            "fmap" : {
                    "__formData": null,
                    "__formData|S|0x20": 8.0
                }
        }
        ));

        EXPECT_NOT_NIL(obj);

        auto shouldSucceed = [&](const char * path, bool succeed) {
            path_resolving::resolve(tes_context::instance(), obj, path, [&](Item * item) {
                EXPECT_TRUE(succeed == (item != nullptr));
            });
        };

        path_resolving::resolve(tes_context::instance(), obj, ".glossary.GlossDiv", [&](Item * item) {
            EXPECT_TRUE(item && item->strValue() && strcmp(item->strValue(), "S") == 0);
        });

        path_resolving::resolve(tes_context::instance(), obj, ".array[0][0]", [&](Item * item) {
            EXPECT_TRUE(item && strcmp(item->strValue(), "NPC Head [Head]") == 0);
        });

        path_resolving::resolve(tes_context::instance(), obj, ".fmap[__formData|S|0x20]", [&](Item * item) {
            EXPECT_TRUE(item && item->isEqual(8.f));
        });

        shouldSucceed(".nonExistingKey", false);
        shouldSucceed(".array[[]", false);
        shouldSucceed(".array[", false);
        shouldSucceed("..array[", false);
        shouldSucceed(".array.[", false);

        shouldSucceed(".array.key", false);
        shouldSucceed("[0].key", false);
    }

    TEST(path_resolving, explicit_key_construction)
    {
        object_base* obj = tes_object::object<map>();

        const char *path = ".keyA.keyB.keyC";

        EXPECT_TRUE( tes_object::solveSetter<SInt32>(obj, path, 14, true) );
        EXPECT_TRUE( tes_object::hasPath(obj, path) );
        EXPECT_TRUE(tes_object::resolveGetter<SInt32>(obj, path) == 14);
    }

    TEST(json_handling, objectFromPrototype)
    {
        auto obj = tes_object::objectFromPrototype("{ \"timesTrained\" : 10, \"trainers\" : [] }")->as<map>();

        EXPECT_TRUE(obj != nullptr);

        path_resolving::resolve(tes_context::instance(), obj, ".timesTrained", [&](Item * item) {
            EXPECT_TRUE(item && item->intValue() == 10);
        });

        path_resolving::resolve(tes_context::instance(), obj, ".trainers", [&](Item * item) {
            EXPECT_TRUE(item && item->object()->as<array>());
        });
    }

    TEST(tes_object, tag)
    {
        object_stack_ref obj = tes_object::object<map>();

        object_stack_ref obj2 = tes_object::object<map>();
        tes_object::retain(obj2);
        EXPECT_TRUE(obj2->_tes_refCount == 1);

        EXPECT_TRUE(obj->_tes_refCount == 0);
        tes_object::retain(obj, "uniqueTag");
        tes_object::retain(obj, "uniqueTag");
        EXPECT_TRUE(obj->_tes_refCount == 2);

        tes_object::releaseObjectsWithTag("uniqueTag");
        EXPECT_TRUE(obj->_tes_refCount == 0);

        // expect that obj2 ref. count left unmodified
        EXPECT_TRUE(obj2->_tes_refCount == 1);
    }

    TEST(tes_map, nextKey)
    {
        map *m = json_deserializer::object_from_json_data(tes_context::instance(), STR({ "A":0, "B" : 1, "Z" : 2 }))->as<map>();

        auto key = tes_map_ext::nextKey<std::string>(m);
        auto itr = m->u_container().begin();
        while (!key.empty()) {
            EXPECT_TRUE(key == itr->first);
            ++itr;
            key = tes_map_ext::nextKey<std::string>(m, key.c_str());
        }
        EXPECT_TRUE(itr == m->u_container().end())
    }

    TEST(tes_object, temp_location)
    {
        tes_context::instance().clearState();

        object_base *obj = tes_object::object<map>();
        //obj->set_tag("temp_location_test");
        tes_object::addToPool(object_stack_ref(obj), "locationA");
        auto id = obj->public_id();

        EXPECT_TRUE(obj->_refCount == 1);
        EXPECT_TRUE(obj->_stack_refCount == 0);

        tes_object::cleanPool("locationA");

        std::this_thread::sleep_for(std::chrono::seconds(16));

        auto foundObj = tes_context::instance().getObject(id);
        EXPECT_TRUE(!foundObj/* || !foundObj->has_equal_tag("temp_location_test")*/);
    }
}

namespace tes_api_3 {

}

#endif
