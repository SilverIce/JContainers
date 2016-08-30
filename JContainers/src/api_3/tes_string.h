#pragma once

#include "util/util.h"
#include "util/stl_ext.h"

namespace collections {

	extern boost::optional<std::vector<std::string>> wrap_string(const char *csource, int charsPerLine);

    class tes_string : public reflection::class_meta_mixin_t<tes_string> {
    public:

        tes_string() {
            metaInfo._className = "JString";
            metaInfo.comment = "various string utility methods";
        }

        static object_base * wrap(tes_context& ctx, const char* source, SInt32 charsPerLine) {

			auto strings = wrap_string(source, charsPerLine);

            if (!strings) {
                return nullptr;
            }

            return &array::objectWithInitializer([&](array &obj) {

                for (auto& str : *strings) {
                    obj._array.push_back(item(str));
                }
            },
                ctx);
        }
        REGISTERF2(wrap, "sourceText charactersPerLine=60",
"Breaks source text onto set of lines of almost equal size.\n\
Returns JArray object containing lines.\n\
Accepts ASCII and UTF-8 encoded strings only");

        static UInt32 decodeFormStringToFormId(const char* form_string) {
            return util::to_integral(decodeFormStringToForm(form_string));
        }
        static FormId decodeFormStringToForm(const char* form_string) {
            return boost::get_optional_value_or(forms::from_string(form_string), FormId::Zero);
        }
        static skse::string_ref encodeFormToString(FormId id) {
            return skse::string_ref{ boost::get_optional_value_or(forms::to_string(id), "") };
        }
        static skse::string_ref encodeFormIdToString(UInt32 id) {
            return encodeFormToString( util::to_enum<FormId>(id) );
        }

        REGISTERF2_STATELESS(decodeFormStringToFormId, "formString", "FormId|Form <-> \"__formData|<pluginName>|<lowFormId>\"-string converisons");
        REGISTERF2_STATELESS(decodeFormStringToForm, "formString", "");
        REGISTERF2_STATELESS(encodeFormToString, "value", "");
        REGISTERF2_STATELESS(encodeFormIdToString, "formId", "");
    };

    TES_META_INFO(tes_string);


#ifndef TEST_COMPILATION_DISABLED

    TEST(tes_string, test)
    {
        tes_context_standalone ctx;

        auto testData = json_deserializer::json_from_file(
            util::relative_to_dll_path("test_data/tes_string/string_wrap.json").generic_string().c_str() );
        EXPECT_TRUE( json_is_array(testData.get()) );

		auto testWrap = [&](const char *string, int linesCount, int charsPerLine) {
            auto obj = tes_string::wrap(ctx, string, charsPerLine);
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