#pragma once

#include "gtest.h"

#include <sstream>
#include <memory>

#ifndef NO_TESTS

namespace collections {
    // static unsigned long create(StaticFunctionTag *) {

    TEST(object_base, refCount)
    {
        auto obj = new array;
        EXPECT_TRUE(obj->refCount() == 1);
        obj->retain();
        EXPECT_TRUE(obj->refCount() == 2);

        obj->release();
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

    TEST(json_parsing, readJSONData)
    {
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

        cJSON *cjson = cJSON_Parse(jsonString);

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


   
/*
    TEST(saveAll, test)
    {
        saveAll();
    }*/

    TEST(array,  test)
    {
        using namespace tes_array;

        _DMESSAGE(__FUNCTION__ " begin");

        HandleT arr = create<array>(0);

        EXPECT_TRUE(count(0, arr) == 0);

        auto str = 10;
        add<SInt32>(0, arr, str);
        EXPECT_TRUE(count(0, arr) == 1);
        EXPECT_TRUE( itemAtIndex<SInt32>(0, arr, 0) == 10);

        str = 30;
        add<SInt32>(0, arr, str);
        EXPECT_TRUE(count(0, arr) == 2);
        EXPECT_TRUE(itemAtIndex<SInt32>(0, arr, 1) == 30);

        HandleT arr2 = create<array>(0);
        add<SInt32>(0, arr2, 4);

        add<Handle>(0, arr, arr2);
        EXPECT_TRUE(itemAtIndex<Handle>(0, arr, 2) == arr2);

        release(0, arr);

        EXPECT_TRUE(itemAtIndex<SInt32>(0, arr2, 0) == 4);

        release(0, arr2);

        _DMESSAGE(__FUNCTION__ " end");
        
    }

    //template<> bool array<Float32>::registerFuncs(VMClassRegistry* registry);
}

#endif

