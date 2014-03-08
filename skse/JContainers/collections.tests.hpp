#pragma once
#include <thread>

namespace collections {

    #ifndef TEST_COMPILATION_DISABLED

    TEST(object_base, refCount)
    {
        auto obj = array::create();
        EXPECT_TRUE(obj->refCount() == 1);
        obj->retain();
        EXPECT_TRUE(obj->refCount() == 2);

        obj->release();
        EXPECT_TRUE(obj->refCount() == 1);

        obj->tes_retain();
        obj->tes_retain();
        EXPECT_TRUE(obj->refCount() == 1 + 2);
        for (int i = 0; i < 20 ; i++) {
        	obj->tes_release();
        }
        EXPECT_TRUE(obj->refCount() == 1);

        obj->release();
    }

/*
    TEST(autorelease_queue, qqq)
    {
        auto& queue = autorelease_queue::instance();

        queue.push(map::create());
        queue.push(array::create());

        EXPECT_TRUE(queue.count() == 2);

        std::this_thread::sleep_for(chrono::seconds(autorelease_queue::obj_lifetime + 10));

        EXPECT_TRUE(queue.count() == 0);

         autorelease_queue::instance().setPaused(false);
    }*/


    TEST_DISABLED(autorelease_queue, high_level)
    {
        using namespace std;

        auto& context = tes_context::instance();

        vector<Handle> identifiers;
        //int countBefore = queue.count();

        for (int i = 0; i < 10; ++i) {
            auto obj = map::object();

            identifiers.push_back(obj->id);
        }

        bool allExist = std::all_of(identifiers.begin(), identifiers.end(), [&](Handle id) {
            return context.getObject(id);
        });

        EXPECT_TRUE(allExist);

        //EXPECT_TRUE(queue.count() == (countBefore + identifiers.size()));

        std::this_thread::sleep_for( std::chrono::seconds(20) );

        bool allDeleted = std::all_of(identifiers.begin(), identifiers.end(), [&](Handle id) {
            return  context.getObject(id) == nullptr;
        });

        EXPECT_TRUE(allDeleted);
    }


#define STR(...)    #__VA_ARGS__

    const char * jsonTestString() {
        const char *jsonString = STR(
        {
            "glossary": {
                "title": "example glossary",
                    "GlossDiv": {
                        "title": "S",
                            "GlossList": {
                                "GlossEntry": {
                                    "ID": "SGML",
                                        "SortAs": "SGML",
                                        "GlossTerm": "Standard Generalized Markup Language",
                                        "Acronym": "SGML",
                                        "Abbrev": "ISO 8879:1986",
                                        "GlossDef": {
                                            "para": "A meta-markup language, used to create markup languages such as DocBook.",
                                                "GlossSeeAlso": ["GML", "XML"]
                                    },
                                        "GlossSee": "markup"
                                }
                        }
                }
            },

                "array": [["NPC Head [Head]", 0, -0.330000]]
        }

        );

        return jsonString;
    }

    TEST(json_handling, readJSONData)
    {


        cJSON *cjson = cJSON_Parse(jsonTestString());

        object_base *obj = json_handling::readCJSON(cjson);
        cJSON * cjson2 = json_handling::createCJSON(*obj);

        char *data1 = cJSON_Print(cjson);
        char *data2 = cJSON_Print(cjson2);

        //EXPECT_TRUE(strcmp(data2, data1) == 0);

        json_handling::resolvePath(obj, ".glossary.GlossDiv.title", [&](Item * item) {
            EXPECT_TRUE(item && strcmp(item->strValue(), "S") == 0 );
        });

        json_handling::resolvePath(obj, ".array[0][0]", [&](Item * item) {
            EXPECT_TRUE(item && strcmp(item->strValue(), "NPC Head [Head]") == 0 );
        });

        float floatVal = 10.5;
        json_handling::resolvePath(obj, ".glossary.GlossDiv.title", [&](Item * item) {
            EXPECT_TRUE(item != nullptr);
            item->setFlt(floatVal);
        });

     //   json_parsing::solv

        free(data1);
        free(data2);
        
        cJSON_Delete(cjson2);
        cJSON_Delete(cjson);
        

    }

    TEST(json_handling, objectFromPrototype)
    {
        object_base *obj = tes_object::objectFromPrototype("{ \"timesTrained\" : 10, \"trainers\" : [] }");

        EXPECT_TRUE(obj->as<map>() != nullptr);
        
        json_handling::resolvePath(obj, ".timesTrained", [&](Item * item) {
            EXPECT_TRUE(item && item->intValue() == 10 );
        });

        json_handling::resolvePath(obj, ".trainers", [&](Item * item) {
            EXPECT_TRUE(item && item->object()->as<array>() );
        });
    }

    TEST(Item, isEqual)
    {
        Item i1, i2;

        EXPECT_TRUE(i1.isEqual(i1));
        EXPECT_TRUE(i1.isEqual(i2));

        i1.setStringVal("me");
        i2.setStringVal("me");
        EXPECT_TRUE(i1.isEqual(i2));

        i2.setStringVal("not me");
        EXPECT_FALSE(i1.isEqual(i2));

        i1 = Item(1);
        i2 = Item(1);
        EXPECT_TRUE(i1.isEqual(i2));

        i2 = Item(1.5f);
        EXPECT_FALSE(i1.isEqual(i2));
    }
   
/*
    TEST(saveAll, test)
    {
        saveAll();
    }*/

    TEST(array,  test)
    {
        auto arr = array::create();

        EXPECT_TRUE(tes_array::count(arr) == 0);

        auto str = 10;
        tes_array::add<SInt32>(arr, str);
        EXPECT_TRUE(tes_array::count(arr) == 1);
        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(arr, 0) == 10);

        str = 30;
        tes_array::add<SInt32>( arr, str);
        EXPECT_TRUE(tes_array::count(arr) == 2);
        EXPECT_TRUE(tes_array::itemAtIndex<SInt32>(arr, 1) == 30);

        auto arr2 = array::create();
        tes_array::add<SInt32>(arr2, 4);

        tes_array::add(arr, arr2);
        EXPECT_TRUE(tes_array::itemAtIndex<Handle>(arr, 2) == arr2->id);

        tes_object::release(arr);

        EXPECT_TRUE(tes_array::itemAtIndex<SInt32>( arr2, 0) == 4);

        tes_object::release( arr2);
    }

    TEST(tes_jcontainers, tes_jcontainers)
    {
        EXPECT_TRUE(tes_jcontainers::isInstalled());

        EXPECT_TRUE(tes_jcontainers::fileExistsAtPath("SafetyLoad.log"));
        EXPECT_TRUE(!tes_jcontainers::fileExistsAtPath("abracadabra"));
    }

    TEST(map, key_case_insensitivity)
    {
        map *cnt = map::object();

        const char *name = "back in black";
        cnt->setValueForKey("ACDC", Item(name));

        EXPECT_TRUE(strcmp(cnt->find("acdc")->strValue(), name) == 0);
    }


    TEST(json_handling, recursion)
    {
        map *cnt = map::object();
        cnt->setValueForKey("cycle", Item(cnt));

        char *data = json_handling::createJSONData(*cnt);

        free(data);
    }

 
    #endif
}

