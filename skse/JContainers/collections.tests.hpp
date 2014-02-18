#pragma once

#include "gtest.h"

#include <sstream>
#include <memory>

namespace collections {
    // static unsigned long create(StaticFunctionTag *) {

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

    TEST_DISABLED(autorelease_queue, qqq)
    {
        bshared_mutex mt;
        autorelease_queue queue(mt);
        queue.start();
        queue.push(10);
        queue.push(20);
        EXPECT_TRUE(queue.count() == 2);
        std::this_thread::sleep_for(chrono::seconds(autorelease_queue::obj_lifetime + 2));

        EXPECT_TRUE(queue.count() == 0);
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

    TEST_DISABLED(json_parsing, readJSONData)
    {


        cJSON *cjson = cJSON_Parse(jsonTestString());

        object_base *obj = json_parsing::readCJSON(cjson);
        cJSON * cjson2 = json_parsing::createCJSON(*obj);

        char *data1 = cJSON_Print(cjson);
        char *data2 = cJSON_Print(cjson2);

        //EXPECT_TRUE(strcmp(data2, data1) == 0);

        json_parsing::resolvePath(obj, ".glossary.GlossDiv.title", [&](Item * item) {
            EXPECT_TRUE(item && strcmp(item->strValue(), "S") == 0 );
        });

        json_parsing::resolvePath(obj, ".array[0][0]", [&](Item * item) {
            EXPECT_TRUE(item && strcmp(item->strValue(), "NPC Head [Head]") == 0 );
        });

        float floatVal = 10.5;
        json_parsing::resolvePath(obj, ".glossary.GlossDiv.title", [&](Item * item) {
            EXPECT_TRUE(item != nullptr);
            item->setFlt(floatVal);
        });

     //   json_parsing::solv

        free(data1);
        free(data2);
        
        cJSON_Delete(cjson2);
        cJSON_Delete(cjson);
        

    }

    TEST(json_parsing, objectFromPrototype)
    {
        object_base *obj = tes_object::objectFromPrototype("{ \"timesTrained\" : 10, \"trainers\" : [] }");

        EXPECT_TRUE(obj->as<map>() != nullptr);
        
        json_parsing::resolvePath(obj, ".timesTrained", [&](Item * item) {
            EXPECT_TRUE(item && item->intValue() == 10 );
        });

        json_parsing::resolvePath(obj, ".trainers", [&](Item * item) {
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
        auto arr = tes_object::create<array>();

        EXPECT_TRUE(tes_array::count(arr) == 0);

        auto str = 10;
        tes_array::add<SInt32>(arr, str);
        EXPECT_TRUE(tes_array::count(arr) == 1);
        EXPECT_TRUE( tes_array::itemAtIndex<SInt32>(arr, 0) == 10);

        str = 30;
        tes_array::add<SInt32>( arr, str);
        EXPECT_TRUE(tes_array::count(arr) == 2);
        EXPECT_TRUE(tes_array::itemAtIndex<SInt32>(arr, 1) == 30);

        auto arr2 = tes_object::create<array>();
        tes_array::add<SInt32>(arr2, 4);

        tes_array::add<Handle>(arr, arr2->id);
        EXPECT_TRUE(tes_array::itemAtIndex<Handle>(arr, 2) == arr2->id);

        tes_object::release(arr);

        EXPECT_TRUE(tes_array::itemAtIndex<SInt32>( arr2, 0) == 4);

        tes_object::release( arr2);
    }

    TEST(tes_jcontainers, tes_jcontainers)
    {
        EXPECT_TRUE(tes_jcontainers::isInstalled());


        const char *path1 = "I:/Games/Elder Scrolls Skyrim/Data/SKSE/Plugins/SafetyLoad.log";
        const char *path2 = "SafetyLoad.log";
        EXPECT_TRUE(tes_jcontainers::fileExistsAtPath(path2));
    }

    #endif
}

