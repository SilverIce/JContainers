#pragma once

#include <string>
#include <optional>

#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/optional.hpp>
#include "boost_extras.h"

#include "skse/skse.h"
#include "util/cstring.h"

namespace forms {

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
        return ((uint64_t)handle >> 32) == 0x0000FFFF;
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

//--------------------------------------------------------------------------------------------------

/**
 * Obtains form id from considering the file (if any) from which it originates.
 *
 * This method tries to implement the conventional Papyrus `GetFormFromFile` function, not handled
 * currently by SKSE. An enhancement is made to return dynamic/global form id, if the incoming file
 * name is empty.
 *
 * @note SKSE returns 8-bit index for ESL mods, though the space for them reserved is 12-bits.
 *
 * @param file name of the mod (e.g. "Skyrim.esm", "The war of green turtles.esl" and etc.)
 * @param form identifier (bits occupying mod ones will be ignored)
 * @return the absolute form id, if any
 */

std::optional<std::uint32_t> form_from_file (std::string_view const& file, std::uint32_t form)
{
    using namespace std;

    if (file.empty ())
        return 0xff000000u | form;

    if (optional<uint8_t> ndx = skse::loaded_mod_index (file))
    {
        return (uint32_t (*ndx) << 24) | (0x00ffffffu & form);
    }

    if (optional<uint8_t> ndx = skse::loaded_light_mod_index (file))
    {
       return 0xfe000000u | (uint32_t (*ndx) << 12) | (0x00000fffu & form);
    }

    return nullopt;
}

//--------------------------------------------------------------------------------------------------

/**
 * Deduce a form identifier out of proper string.
 *
 * Several options are available:
 *
 * * `[__formData|]<mod name>|<relative id>`
 *   Search for mod name form prefix and append the given relative, 24-bit for esp/esm and
 *   12-bit for esl, identifier. Any incoming id bits in the place of the mod bits will be
 *   ignored (e.g. `__formData|test.esl|0x006780004` is about form #4).
 *
 * * `[__formData|]|<dynamic form id>`
 *   Same as above, but as the mod name is missing, assume the form identifier is about dynamic
 *   forms. In practice the most significant bits will be set always.
 *
 * @note The `__formData|` prefix is optional.
 *
 * @param pstr - can be nullptr or empty too. Assumed it is alive during the duration of this call.
 * @return optional absolute form identifier, converted from the passed string.
 */

std::optional<std::uint32_t> form_from_file (const char* pstr)
{
    using namespace std;

    if (!pstr)
        return nullopt;

    string_view str (pstr);

    constexpr char prefix[] = "__formData|";
    if (str.find (prefix) == 0)
        str.remove_prefix (sizeof prefix - 1);

    auto mpos = str.find ('|');
    if (mpos == string_view::npos)
        return nullopt;

    string_view const mod = str.substr (0, mpos);
    string_view const fid = str.substr (mpos + 1);

    uint32_t form;
    try {
        form = stoul (string (fid), nullptr, 0); // likely std::string on stack (aka SSO)
    }
    catch (...) {
        return nullopt;
    }

    return form_from_file (mod, form);
}

//--------------------------------------------------------------------------------------------------

    inline boost::optional<FormId> from_string (const char* pstr)
    {
        auto pair1 = bs::half_split(fstring, kFormDataSeparator);

        if (pair1.second.empty() || !std::equal(pair1.first.begin(), pair1.first.end(), kFormData)) {
            return boost::none;
        }

        auto pair2 = bs::half_split(pair1.second, kFormDataSeparator);
        // pair2.first - modname part can be empty
        if (/*pair2.first.empty() || */pair2.second.empty()) {
            return boost::none;
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
        return from_string(util::make_cstring_safe(source, 1024));
    }
}
