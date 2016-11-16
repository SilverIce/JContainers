#pragma once

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

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
                    obj.u_container().emplace_back(std::move(str));
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

    private:
        static boost::uuids::random_generator generateUUID_gen;
        static util::spinlock generateUUID_lock;

    public:
        static std::string generateUUID() {
            return boost::uuids::to_string(
                util::perform_while_locked(generateUUID_lock, [](){ return generateUUID_gen(); })
            );
        }
        REGISTERF2_STATELESS(generateUUID, "", "Generates random uuid-string like 2e80251a-ab22-4ad8-928c-2d1c9561270e");

    };

    boost::uuids::random_generator  tes_string::generateUUID_gen;
    util::spinlock                  tes_string::generateUUID_lock;


    TES_META_INFO(tes_string);

    TEST(tes_string, wrap)
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

    TEST(tes_string, generateUUID)
    {
        auto uidString = tes_string::generateUUID();
        EXPECT_FALSE(uidString.empty());

        auto uidString2 = tes_string::generateUUID();
        EXPECT_NE(uidString, uidString2);
    }
}