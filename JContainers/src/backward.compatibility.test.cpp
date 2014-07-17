#if 0
#include <fstream>
#include "gtest.h"

#include "jcontainers_constants.h"
#include "tes_context.h"
#include "json_handling.h"

namespace collections {

    struct testing_compatibility : testing::Fixture {

        void test() {
            // will fail for now
            //compare_with_current_snapshot();

            // will fail anyway
            //compare_serialized_data("jdb.json", "serialized_0.66.1");
        }

        // make snapshot of jdbfile, then load it, serialize to currJson and compare with original json
        void compare_with_current_snapshot() {
            auto currSnapshot = make_snapshot_of_current_version();
            auto currJson = load_json_from_shapshot_data(currSnapshot);

            auto textjson = json_deserializer::json_from_file(jdb_file());

            EXPECT_TRUE( json_equal(currJson.get(), textjson.get()) == 1 );
        }

        void compare_serialized_data(const char * jsonFilePath, const char *serializedDataPath) {

            auto serializedData = load_json_from_shapshot_file(serializedDataPath);
            auto textjson = json_deserializer::json_from_file(jsonFilePath);

            EXPECT_TRUE( json_equal(serializedData.get(), textjson.get()) == 1 );
        }


        static const char *jdb_file() {
            return "jdb.json";
        }

        static std::string current_version_string() {
            return std::string("serialized_") + std::to_string(kJVersionMajor) + "." +  std::to_string(kJVersionMinor);
        }

        std::string make_snapshot_of_current_version() {
            tes_context ctx;

            auto root = json_deserializer::object_from_file(ctx, jdb_file());
            EXPECT_NOT_NIL( root );
            ctx.setDataBase(root);
            auto data = ctx.write_to_string();
            ctx.clearState();

            return data;
        }

        void write_snapshot_of_current_version() {

            auto filename = current_version_string();
            auto data = make_snapshot_of_current_version();

            EXPECT_NIL( make_unique_file(fopen(filename.c_str(), "rb")).get() );

            fwrite(data.c_str(), 1, data.size(),
                make_unique_file(fopen(
                    filename.c_str()
                    ,"wb")
                ).get()
            );
        }

        json_unique_ref load_json_from_shapshot_file(const char *serializedDataPath) {

            std::ifstream file(serializedDataPath, std::ios::in | std::ios::binary);
            std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            return load_json_from_shapshot_data(data);
        }

        json_unique_ref load_json_from_shapshot_data(const std::string& snapshot) {

            tes_context ctx;

            ctx.read_from_string(snapshot, kJSerializationCurrentVersion);
            auto jvalue = json_serializer::create_json_value(*ctx.database());
            EXPECT_NOT_NIL(jvalue);

            ctx.clearState();

            return jvalue;
        }



    };

    TEST_F_CUSTOM_CLASS(testing_compatibility, t);
}

#endif
