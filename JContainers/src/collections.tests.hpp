#pragma once

#include <future>

namespace collections {

    struct JCFixture : testing::Fixture {
        tes_context context;

        ~JCFixture() {
            context.clearState();
        }

    };

#   define JC_TEST(name, name2) TEST_F(JCFixture, name, name2)
#   define JC_TEST_DISABLED(name, name2) TEST_F(JCFixture, name, DISABLED_##name2)

}

#ifndef TEST_COMPILATION_DISABLED

namespace collections {

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

                "array": [["NPC Head [Head]", 0, -0.330000]],

                "fKey": "__formData|Plugin.esp|0x1234"
        }

        );

        return jsonString;
    }

    JC_TEST(object_base, refCount)
    {
        auto obj = array::object(context);
        EXPECT_TRUE(obj->refCount() == 0); // aqueue retains it -- no more

        obj->retain();
        EXPECT_TRUE(obj->refCount() == 1);

        obj->tes_retain();
        obj->tes_retain();
        obj->tes_retain();
        EXPECT_TRUE(obj->refCount() == 1 + 3);

        // ensure that over-release does not affects internal ref count:
        for (int i = 0; i < 20 ; i++) {
            obj->tes_release();
        }
        EXPECT_TRUE(obj->refCount() == 1);

        obj->release();
        EXPECT_TRUE(obj->refCount() == 1); // aqueue retains it

        // that will damage memory, later: 
        //obj->release();
        //EXPECT_TRUE(obj->refCount() == 1);
    }

    JC_TEST(Item, nulls)
    {
        Item i1;

        EXPECT_TRUE(i1.isNull());

        i1 = (const char*)nullptr;
        EXPECT_TRUE(i1.isNull());

        i1 = (TESForm*)nullptr;
        EXPECT_TRUE(i1.isNull());

        i1 = FormZero;
        EXPECT_TRUE(i1.isNull());

        i1 = (object_base *)nullptr;
        EXPECT_TRUE(i1.isNull());
    }

    JC_TEST(Item, equality)
    {
        Item i1, i2;

        EXPECT_TRUE(i1.isNull());
        EXPECT_TRUE(i2.isNull());

        EXPECT_TRUE(i1.isEqual(i1));
        EXPECT_TRUE(i1.isEqual(i2));

        i1 = "me";
        i2 = "me";
        EXPECT_TRUE(i1.isEqual(i2));

        i2 = "not me";
        EXPECT_FALSE(i1.isEqual(i2));

        i1 = 1u;
        i2 = 1u;
        EXPECT_TRUE(i1.isEqual(i2));

        i2 = 1.5f;
        EXPECT_FALSE(i1.isEqual(i2));

        auto obj = array::object(context);
        i1 = obj;
        i2 = obj;
        EXPECT_TRUE(i1.isEqual(i2));
    }

    TEST(form_handling, test)
    {
        namespace fh = form_handling;

        EXPECT_TRUE( fh::is_form_string("__formData|Skyrim.esm|0x1" ));
        EXPECT_FALSE( fh::is_form_string("__formDatttt" ));
        EXPECT_FALSE( fh::is_form_string(nullptr));

        // test static form ids
        {
            const int pluginIdx = 'B';
            const FormId form = (FormId)(pluginIdx << 24 | 0x14);

            EXPECT_TRUE( fh::is_static(form) );
            EXPECT_EQ(form, fh::construct(pluginIdx, 0x14));

            std::string formString = *fh::to_string(form);
            EXPECT_TRUE( formString == 
                (std::string(fh::kFormData) + fh::kFormDataSeparator + skse::modname_from_index(pluginIdx) + fh::kFormDataSeparator + "0x14"));

            EXPECT_TRUE( form == 
                *fh::from_string(formString.c_str()) );

        }

        // test global (0xFF*) form ids
        {
            const FormId form = (FormId)(FormGlobalPrefix << 24 | 0x14);

            EXPECT_TRUE( !fh::is_static(form) );
            EXPECT_EQ(form, fh::construct(FormGlobalPrefix, 0x14));

            std::string formString = *fh::to_string(form);

            EXPECT_TRUE( formString == 
                (std::string(fh::kFormData) + fh::kFormDataSeparator + fh::kFormDataSeparator + "0xff000014"));

            EXPECT_TRUE( form == 
                *fh::from_string(formString.c_str()) );
        }
        {
            const char *unresolveableFString = "__formData|ssa.esm|0x1";

            EXPECT_TRUE( fh::is_form_string(unresolveableFString) );
            EXPECT_FALSE( fh::from_string(unresolveableFString) );

            // is invalid in sythetic test only: all plugin indexes except 'A'-'Z' are invalid
            FormId invalidFormId = (FormId)fh::construct('%', 0x14);
            EXPECT_FALSE( fh::to_string(invalidFormId) );
        }
    }

    JC_TEST(json_deserializer, test)
    {
        EXPECT_NIL( json_deserializer::object_from_file(context, "") );
        EXPECT_NIL( json_deserializer::object_from_file(context, nullptr) );

        EXPECT_NIL( json_deserializer::object_from_json_data(context, "") );
        EXPECT_NIL( json_deserializer::object_from_json_data(context, nullptr) );

        EXPECT_NOT_NIL( json_deserializer::object_from_json_data(context, jsonTestString()) );
    }

    // load json file into tes_context -> serialize into json again -> compare with original json
    struct json_loading_test : testing::Fixture {

        void test() {

            namespace fs = boost::filesystem;

            fs::path dir("test_data/json_loading_test");
            fs::directory_iterator end;
            bool atLeastOneTested = false;

            for (fs::directory_iterator itr(dir); itr != end; ++itr) {
                if (fs::is_regular_file(*itr)) {
                    atLeastOneTested = true;
                    do_comparison(itr->path().generic_string().c_str());
                }
            }

            EXPECT_TRUE( atLeastOneTested );
        }
        
        void do_comparison(const char *file_path) {
            EXPECT_NOT_NIL( file_path );

            tes_context ctx;

            auto root = json_deserializer::object_from_file(ctx, file_path);
            EXPECT_NOT_NIL( root );
            auto jsonOut = json_serializer::create_json_value(*root);
            ctx.clearState();

            auto originJson = json_deserializer::json_from_file(file_path);
            EXPECT_NOT_NIL( originJson );

            EXPECT_TRUE( json_equal(originJson.get(), jsonOut.get()) == 1 ); 
        }
    };

    TEST_F_CUSTOM_CLASS(json_loading_test, t);

    JC_TEST(json_serializer, no_infinite_recursion)
    {
        map *cnt = map::object(context);
        cnt->u_setValueForKey("cycle", Item(cnt));

        json_serializer::create_json_data(*cnt);
    }

    JC_TEST(tes_context, backward_compatibility)
    {
        namespace fs = boost::filesystem;

        fs::path dir("test_data/backward_compatibility");
        fs::directory_iterator end;
        bool atLeastOneTested = false;

        for (fs::directory_iterator itr(dir); itr != end; ++itr) {
            if (fs::is_regular_file(*itr)) {
                atLeastOneTested = true;

                std::ifstream file(itr->path().generic_string(), std::ios::in | std::ios::binary);
                context.read_from_stream(file, kJSerializationCurrentVersion);
            }
        }

        EXPECT_TRUE(atLeastOneTested);
    }

}

// API-related tests:
namespace collections {

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

    }

    TEST(tes_jcontainers, tes_jcontainers)
    {
        EXPECT_TRUE(tes_jcontainers::isInstalled());

        EXPECT_FALSE(tes_jcontainers::fileExistsAtPath(nullptr));
        EXPECT_TRUE(tes_jcontainers::fileExistsAtPath("JContainers.dll"));
        EXPECT_TRUE(!tes_jcontainers::fileExistsAtPath("abracadabra"));
    }

    JC_TEST(map, key_case_insensitivity)
    {
        map *cnt = map::object(context);

        std::string name = "back in black";
        cnt->u_setValueForKey("ACDC", Item(name));

        EXPECT_TRUE(*cnt->u_find("acdc")->stringValue() == name);
    }

    TEST(path_resolving, collection_operators)
    {
        auto shouldReturnNumber = [&](object_base *obj, const char *path, float value) {
            path_resolving::resolvePath(obj, path, [&](Item * item) {
                EXPECT_TRUE(item && item->fltValue() == value);
            });
        };

        auto shouldReturnInt = [&](object_base *obj, const char *path, int value) {
            path_resolving::resolvePath(obj, path, [&](Item * item) {
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
            "array" : [["NPC Head [Head]", 0, -0.330000]]
        }
        ));

        auto shouldSucceed = [&](const char * path, bool succeed) {
            path_resolving::resolvePath(obj, path, [&](Item * item) {
                EXPECT_TRUE(succeed == (item != nullptr));
            });
        };

        path_resolving::resolvePath(obj, ".glossary.GlossDiv", [&](Item * item) {
            EXPECT_TRUE(item && item->strValue() && strcmp(item->strValue(), "S") == 0);
        });

        /*      feature disabled
        json_handling::resolvePath(obj, "glossary.GlossDiv", [&](Item * item) {
        EXPECT_TRUE(item && strcmp(item->strValue(), "S") == 0 );
        });*/

        path_resolving::resolvePath(obj, ".array[0][0]", [&](Item * item) {
            EXPECT_TRUE(item && strcmp(item->strValue(), "NPC Head [Head]") == 0);
        });

        shouldSucceed(".nonExistingKey", false);
        shouldSucceed(".array[[]", false);
        shouldSucceed(".array[", false);
        shouldSucceed("..array[", false);
        shouldSucceed(".array.[", false);

        shouldSucceed(".array.key", false);
        shouldSucceed("[0].key", false);
        /*
        json_handling::resolvePath(obj, ".nonExistingKey", [&](Item * item) {
        EXPECT_TRUE(!item);
        });*/


        float floatVal = 10.5;
        path_resolving::resolvePath(obj, ".glossary.GlossDiv", [&](Item * item) {
            EXPECT_TRUE(item != nullptr);
            *item = floatVal;
        });
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

        path_resolving::resolvePath(obj, ".timesTrained", [&](Item * item) {
            EXPECT_TRUE(item && item->intValue() == 10);
        });

        path_resolving::resolvePath(obj, ".trainers", [&](Item * item) {
            EXPECT_TRUE(item && item->object()->as<array>());
        });
    }

}

namespace collections {

    JC_TEST(tes_context, database)
    {
        using namespace std;

        auto db = context.database();

        EXPECT_TRUE(db != nullptr);
        EXPECT_TRUE(db == context.database());
    }

    JC_TEST(tes_context, serialization)
    {
        using namespace std;

        vector<Handle> identifiers;

        auto allExist = [&]() {
            return std::all_of(identifiers.begin(), identifiers.end(), [&](Handle id) {
                return context.getObject(id) != nullptr;
            });
        };

        auto allAbsent = [&]() {
            return std::all_of(identifiers.begin(), identifiers.end(), [&](Handle id) {
                return context.getObject(id) == nullptr;
            });
        };

        for (int i = 0; i < 10; ++i) {
            auto obj = map::object(context);
            identifiers.push_back(obj->uid());
        }

        EXPECT_TRUE(allExist());

        string data = context.write_to_string();
        EXPECT_TRUE(allExist());
        EXPECT_FALSE(data.empty());

        context.clearState();
        EXPECT_TRUE(allAbsent());

        context.read_from_string(data, kJSerializationCurrentVersion);
        EXPECT_TRUE(allExist());

        string newData = context.write_to_string();
        EXPECT_TRUE(data.size() == newData.size());
    }

    JC_TEST(autorelease_queue, over_release)
    {
        using namespace std;

        vector<Handle> identifiers;
        //int countBefore = queue.count();

        for (int i = 0; i < 10; ++i) {
            auto obj = map::make(context);//rc is 0
            identifiers.push_back(obj->public_id());

            EXPECT_TRUE(obj->refCount() == 0);

            obj->retain();//+1 rc
            EXPECT_TRUE(obj->refCount() == 1);

            obj->release();//-1, rc is 0, add to aqueue, rc is 1
            EXPECT_TRUE(obj->refCount() == 1); // queue owns it now

            obj->retain();// +1, rc is 2
            EXPECT_TRUE(obj->refCount() == 2);
        }

        auto allExist = [&]() {
            return std::all_of(identifiers.begin(), identifiers.end(), [&](Handle id) {
                return context.getObject(id);
            });
        };

        std::this_thread::sleep_for( std::chrono::seconds(14) );

        //
        EXPECT_TRUE(allExist());

        for (Handle id : identifiers) {
            auto obj = context.getObject(id);
            EXPECT_TRUE(obj->refCount() == 1);
        }
    }

    JC_TEST(autorelease_queue, ensure_destroys)
    {
        using namespace std;

        vector<Handle> public_identifiers, privateIds;

        for (int i = 0; i < 10; ++i) {
            auto obj = map::object(context);
            public_identifiers.push_back(obj->uid());

            auto priv = map::make(context);
            privateIds.push_back(priv->public_id());
            priv->prolong_lifetime();
        }

        auto allExist = [&](vector<Handle>& identifiers) {
            return std::all_of(identifiers.begin(), identifiers.end(), [&](Handle id) {
                return context.getObject(id);
            });
        };

        auto allDestroyed = [&](vector<Handle>& identifiers) {
            return std::all_of(identifiers.begin(), identifiers.end(), [&](Handle id) {
                return !context.getObject(id);
            });
        };


        std::this_thread::sleep_for( std::chrono::seconds(9) );

        EXPECT_FALSE(allDestroyed(privateIds));
        EXPECT_TRUE(allExist(public_identifiers));

        std::this_thread::sleep_for( std::chrono::seconds(5) );

        EXPECT_TRUE(allExist(public_identifiers) == false);
    }

    JC_TEST_DISABLED(dl, dl)
    {
        auto obj = map::object(context);
        object_lock lock(obj);
        obj->u_setValueForKey("lol", Item(obj));
    }

    JC_TEST_DISABLED(deadlock, deadlock)
    {
        map::ref root = map::object(context);
        map *elsa = map::object(context);
        map *local = map::object(context);

        root->setValueForKey("elsa", Item(elsa));
        elsa->setValueForKey("local", Item(local));

        auto func = [&]() {
            printf("work started\n");

            auto rootId = root->tes_uid();
            auto elsaId = elsa->uid();

            int i = 0;
            while (i < 5000000) {
                ++i;
                auto root = context.getObjectOfType<map>(rootId);
                auto elsa = tes_object::resolveGetter<object_base *>(root, ".elsa");

                tes_object::hasPath(root, ".elsa.local");
                tes_map::setItem(root, "elsa", elsa);
            }

            printf("work complete\n");
        };

        auto fut1 = std::async(std::launch::async, func);
        auto fut2 = std::async(std::launch::async, func);

        fut1.wait();
        fut2.wait();
    }
}

#endif
