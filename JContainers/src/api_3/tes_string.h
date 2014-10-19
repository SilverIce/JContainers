#pragma once

#include "util.h"

namespace collections {

	extern boost::optional<std::vector<std::string>> wrap_string(const char *csource, int charsPerLine);

    class tes_string : public reflection::class_meta_mixin_t<tes_string> {
    public:

        tes_string() {
            metaInfo._className = "JString";
            metaInfo.comment = "various string utility methods";
        }

        static object_base * wrap(const char* source, SInt32 charsPerLine) {

			auto strings = wrap_string(source, charsPerLine);

            if (!strings) {
                return nullptr;
            }

            return &array::objectWithInitializer([&](array &obj) {

                for (auto& str : *strings) {
                    obj._array.push_back(Item(str));
                }
            },
                tes_context::instance());
        }
        REGISTERF2(wrap, "sourceText charactersPerLine=60",
"Breaks source text onto set of lines of almost equal size.\n\
Returns JArray object containing lines.\n\
Accepts ASCII and UTF-8 encoded strings only");

    };

    TES_META_INFO(tes_string);


#ifndef TEST_COMPILATION_DISABLED

    TEST(tes_string, test)
    {
        auto testData = json_deserializer::json_from_file(
            util::relative_to_dll_path("test_data/tes_string/string_wrap.json").generic_string().c_str() );
        EXPECT_TRUE( json_is_array(testData.get()) );

		auto testWrap = [&](const char *string, int linesCount, int charsPerLine) {
            auto obj = tes_string::wrap(string, charsPerLine);
            if (linesCount == -1) {
                EXPECT_NIL(obj);
            }
            else {
                EXPECT_NOT_NIL(obj);
                EXPECT_TRUE(obj->s_count() >= linesCount);
            }
		};
        
        size_t index = 0;
        json_t *value = nullptr;
        json_array_foreach(testData.get(), index, value) {
            int charsPerLine = -1;
            json_t *jtext = nullptr;
            int linesCountMinimum = -1;

            json_error_t error;
            int succeed = json_unpack_ex(value, &error, 0,
                "{s:i, s:o, s:i}", "charsPerLine", &charsPerLine, "text", &jtext, "linesCountMinimum", &linesCountMinimum);
            EXPECT_TRUE(succeed == 0);

            testWrap(json_string_value(jtext), linesCountMinimum, charsPerLine);
        }
    }

#endif

}