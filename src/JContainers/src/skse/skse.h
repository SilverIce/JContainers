#pragma once

#include "forms/form_id.h"
#include <cstdint>
#include <string_view>
#include <optional>

class TESForm;

/// Wraps calls to SKSE - fakes the calls when SKSE/Skyrim inactive (during synthetic tests)
namespace skse
{

/**
 * Forwards to SKSE `GetLoadedModIndex`.
 * @returns the mod index if found, 0 if silent API, name[0] (if A-Z) for test API.
 */

std::optional<std::uint8_t> loaded_mod_index (string_view const& name);

/**
 * Forwards to SKSE `GetLoadedLightModIndex`.
 * @returns the mod index if found, 0 if silent API, name[0] (if A-Z) for test API.
 */

std::optional<std::uint8_t> loaded_light_mod_index (string_view const& name);

/**
 * Forwards to SKSE `modList.loadedMods[idx]->name`.
 * @returns the mod name or nullptr, empty string if silent API, idx as char* (if A-Z) for test API.
 */

const char* modname_from_index (std::uint8_t idx);

/**
 * Forwards to SKSE `GetModIndex`.
 * @returns the mod name or 0xFF, 0 if silent API, name[0] (if A-Z) for test API.
 */

std::uint8_t modindex_from_name (const char* name);

    forms::FormId resolve_handle(forms::FormId handle);
    TESForm* lookup_form(forms::FormId handle);

    bool try_retain_handle(forms::FormId handle);
    void release_handle(forms::FormId handle);

    void console_print(const char * fmt, ...);
    void console_print(const char * fmt, const va_list& args);

    void set_real_api();
    void set_fake_api();
    void set_silent_api();
}
