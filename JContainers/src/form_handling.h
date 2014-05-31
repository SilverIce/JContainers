#pragma once

#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/find_end.hpp>
#include <boost/optional.hpp>

#include "skse/GameData.h"
#include "collections.h"

namespace collections {

    namespace form_handling {

        static const char * kFormData = "__formData";
        static const char * kFormDataSeparator = "|";

        inline uint8_t mod_index(FormId formId) {
            return formId >> 24;
        }

        inline bool is_static(FormId formId) {
            return mod_index(formId) != FormGlobalPrefix;
        }

        inline uint32_t local_id(FormId formId) {
            return formId & 0x00FFFFFF;
        }

        inline FormId construct(uint8_t mod_id, uint32_t local_identifier) {
            return (FormId) ((mod_id << 24) | local_id((FormId)local_identifier));
        }

        template<class T>
        inline boost::optional<std::string> to_string(FormId formId, T modNameFunc) {

            auto modID = mod_index(formId);
            uint32_t formIdClean = formId;

            const char * modName = nullptr;

            if (is_static(formId)) { // common case
                modName = modNameFunc(modID);
                if (!modName) {
                    return boost::make_optional(false, std::string("invalid form id"));
                }

                formIdClean = local_id(formId);
            }
            else {
                // global form is not bound to any plugin
                modName = "";
            }

            std::string string = kFormData;
            string += kFormDataSeparator;
            string += modName;
            string += kFormDataSeparator;

            char buff[20] = {'\0'};
            sprintf(buff, "0x%x", formIdClean);
            string += buff;

            return string;
        }

        // TODO: rename me!
        template<class T>
        inline boost::optional<FormId> from_string(const char* source, T modIndexFunc) {

            if (!source) {
                return boost::optional<FormId>(false, FormZero);
            }

            namespace bs = boost;
            namespace ss = std;

            auto fstring = bs::make_iterator_range(source, source + strnlen_s(source, 1024));

            if (!bs::starts_with(fstring, kFormData)) {
                return boost::optional<FormId>(false, FormZero);
            }

            ss::vector<decltype(fstring)> substrings;
            bs::split(substrings, source, bs::is_any_of(kFormDataSeparator));

            if (substrings.size() != 3) {
                return boost::optional<FormId>(false, FormZero);
            }

            auto& pluginName = substrings[1];

            UInt8 modIdx = 0;
            if (!pluginName.empty()) {
                modIdx = modIndexFunc( ss::string(pluginName.begin(), pluginName.end()).c_str() );
                if (modIdx == FormGlobalPrefix) {
                    return boost::optional<FormId>(false, FormZero);
                }
            }
            else {
                // 
                modIdx = FormGlobalPrefix;
            }

            auto& formIdString = substrings[2];

            uint32_t formId = 0;
            try {
                formId = std::stoul(ss::string(formIdString.begin(), formIdString.end()), nullptr, 0);
            }
            catch (const std::invalid_argument& ) {
                return boost::optional<FormId>(false, FormZero);
            }
            catch (const std::out_of_range& ) {
                return boost::optional<FormId>(false, FormZero);
            }

            formId = construct(modIdx, formId);
            return (FormId)formId;
        }

        inline boost::optional<std::string> to_string(FormId formId) {
            return to_string(formId, [](UInt8 modId) {
                DataHandler * dhand = DataHandler::GetSingleton();
                ModInfo * modInfo = dhand->modList.loadedMods[modId];
                return modInfo ? modInfo->name : nullptr;
            });
        }

        // TODO: rename me!
        inline boost::optional<FormId> from_string(const char* source) {
            return from_string(source, [](const char *modName) {
                return DataHandler::GetSingleton()->GetModIndex( modName );
            });
        }
    }

}
