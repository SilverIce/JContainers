#pragma once

#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
//#include <boost/range/algorithm/find_if.hpp>
//#include <boost/range/algorithm/find_end.hpp>
#include <boost/optional.hpp>
#include "boost_extras.h"

#include "skse/GameData.h"
#include "collections.h"
#include "skse.h"

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

        inline FormId resolve_handle(FormId handle) {
            if (is_static(handle)) {
                return (FormId)skse::resolve_handle((uint32_t)handle);
            }
            else {
                return handle;
            }
        }

        inline boost::optional<std::string> to_string(FormId formId) {

            auto modID = mod_index(formId);
            uint32_t formIdClean = formId;

            const char * modName = nullptr;

            if (is_static(formId)) { // common case
                modName = skse::modname_from_index (modID);
                if (!modName) {
                    return false;
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

        inline bool is_form_string(const char *string) {
            return string && strncmp(string, kFormData, strlen(kFormData)) == 0;
        }

        // TODO: rename me!
        inline boost::optional<FormId> from_string(boost::iterator_range<const char*>& fstring) {
            namespace bs = boost;
            namespace ss = std;

            auto pair1 = bs::half_split(fstring, "|");

            if (pair1.second.empty() || !std::equal(pair1.first.begin(), pair1.first.end(), kFormData)) {
                return boost::optional<FormId>(false, FormZero);
            }

            auto pair2 = bs::half_split(pair1.second, "|");
            // pair2.first - modname part can be empty
            if (/*pair2.first.empty() || */pair2.second.empty()) {
                return boost::optional<FormId>(false, FormZero);
            }
            
            auto& pluginName = pair2.first;

            UInt8 modIdx = 0;
            if (!pluginName.empty()) {
                modIdx = skse::modindex_from_name( ss::string(pluginName.begin(), pluginName.end()).c_str() );
                if (modIdx == FormGlobalPrefix) {
                    return boost::optional<FormId>(false, FormZero);
                }
            }
            else {
                // 
                modIdx = FormGlobalPrefix;
            }

            auto& formIdString = pair2.second;

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

        inline boost::optional<FormId> from_string(const char* source) {
            auto fstring = boost::make_iterator_range(source, source + strnlen_s(source, 1024));
            return from_string(fstring);
        }
    }

}
