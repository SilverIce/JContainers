#pragma once

#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/optional.hpp>
#include "boost_extras.h"

#include "skse/skse.h"

namespace collections {

    namespace form_handling {

        inline uint8_t mod_index(FormId formId) {
            return (uint32_t)formId >> 24;
        }

        inline bool is_static(FormId formId) {
            return mod_index(formId) != FormGlobalPrefix;
        }

        inline uint32_t local_id(FormId formId) {
            return (uint32_t)formId & 0x00FFFFFF;
        }

        inline bool is_form_handle(FormHandle handle) {
            return (uint32_t)((uint64_t)handle >> 32) == 0x0000FFFF;
        }

        inline FormId form_handle_to_id(FormHandle handle) {
            return static_cast<FormId>(handle);
        }

        inline FormHandle form_id_to_handle(FormId id) {
            return (FormHandle)(0x0000ffff00000000 | (uint64_t)id);
        }

        inline FormId construct(uint8_t mod_id, uint32_t local_identifier) {
            return (FormId) ((mod_id << 24) | local_id((FormId)local_identifier));
        }

        inline FormId resolve_handle(FormId handle) {
            if (is_static(handle)) {
                return skse::resolve_handle(handle);
            }
            else {
                return skse::lookup_form(handle) ? handle : FormId::Zero;
            }
        }

        static const char kFormData[] = "__formData";
        static const char * kFormDataSeparator = "|";

        inline boost::optional<std::string> to_string(FormId formId) {

            auto modID = mod_index(formId);
            uint32_t formIdClean = (uint32_t)formId;

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

            std::string string{ kFormData };
            string += kFormDataSeparator;
            string += modName;
            string += kFormDataSeparator;

            char buff[20] = {'\0'};
            _snprintf(buff, sizeof buff, "0x%x", formIdClean);
            string += buff;

            return string;
        }

        inline bool is_form_string(const char *string) {
            return string && strncmp(string, kFormData, sizeof kFormData - 1) == 0;
        }

        // TODO: rename me!
        inline boost::optional<FormId> from_string(boost::iterator_range<const char*>& fstring) {
            namespace bs = boost;
            namespace ss = std;

            auto pair1 = bs::half_split(fstring, kFormDataSeparator);

            if (pair1.second.empty() || !std::equal(pair1.first.begin(), pair1.first.end(), kFormData)) {
                return boost::optional<FormId>(false, FormId::Zero);
            }

            auto pair2 = bs::half_split(pair1.second, kFormDataSeparator);
            // pair2.first - modname part can be empty
            if (/*pair2.first.empty() || */pair2.second.empty()) {
                return boost::optional<FormId>(false, FormId::Zero);
            }
            
            auto& pluginName = pair2.first;

            UInt8 modIdx = 0;
            if (!pluginName.empty()) {
                modIdx = skse::modindex_from_name( ss::string(pluginName.begin(), pluginName.end()).c_str() );
                if (modIdx == FormGlobalPrefix) {
                    return boost::none;
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
                return boost::none;
            }
            catch (const std::out_of_range& ) {
                return boost::none;
            }

            return construct(modIdx, formId);
        }

        inline boost::optional<FormId> from_string(const char* source) {
            auto fstring = boost::make_iterator_range(source, source + strnlen_s(source, 1024));
            return from_string(fstring);
        }
    }

}
