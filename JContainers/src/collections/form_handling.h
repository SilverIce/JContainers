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
                    return boost::none;
                }

                formIdClean = local_id(formId);
            }
            else {
                // global form is not bound to any plugin
                modName = "";
            }

            char string[MAX_PATH] = { '\0' };
            assert(-1 != _snprintf_s(string, sizeof string, "__formData|%s|0x%x", modName, formIdClean));

            return boost::optional<std::string>{ string };
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
                char *long_string = (char*)_malloca(pluginName.size() + 1);
                auto res = strncpy_s(long_string, pluginName.size() + 1, pluginName.begin(), pluginName.size());
                assert(0 == res);

                modIdx = skse::modindex_from_name(long_string);
                if (modIdx == FormGlobalPrefix) {
                    return boost::none;
                }
            }
            else {
                // 
                modIdx = FormGlobalPrefix;
            }

            auto& formIdString = pair2.second;

            auto stoul_optimized = [](boost::iterator_range<const char*>& str, size_t *_Idx, int _Base) -> bs::optional<unsigned long> {	// convert string to unsigned long
                char *long_string = (char*)_malloca(str.size() + 1);
                auto res = strncpy_s(long_string, str.size() + 1, str.begin(), str.size());
                assert(0 == res);

                const char *_Ptr = long_string;
                char *_Eptr;
                errno = 0;
                unsigned long _Ans = _CSTD strtoul(_Ptr, &_Eptr, _Base);

                if (_Ptr == _Eptr)
                    return boost::none; //_Xinvalid_argument("invalid stoul argument");
                if (errno == ERANGE)
                    return boost::none; //_Xout_of_range("stoul argument out of range");
                if (_Idx != 0)
                    *_Idx = (size_t)(_Eptr - _Ptr);
                return (_Ans);
            };

            bs::optional<unsigned long> optFormId = stoul_optimized(formIdString, nullptr, 0);
            if (optFormId) {
                return construct(modIdx, *optFormId);
            }

            return boost::none;
        }

        inline boost::optional<FormId> from_string(const char* source) {
            auto fstring = boost::make_iterator_range(source, source + strnlen_s(source, 1024));
            return from_string(fstring);
        }
    }

}
