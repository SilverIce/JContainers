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

std::optional<std::uint8_t> loaded_mod_index (std::string_view const& name);

/**
 * Forwards to SKSE `GetLoadedLightModIndex`.
 * @returns the mod index if found, 0 if silent API, name[0] (if A-Z) for test API.
 */

std::optional<std::uint16_t> loaded_light_mod_index (std::string_view const& name);

/**
 * Forwards to SKSE `modList.loadedMods[idx]->name`.
 * @returns the mod name if found, empty string if silent API, idx as char* (if A-Z) for test API.
 */

std::optional<std::string_view> loaded_mod_name (std::uint8_t idx);

/**
 * Forwards to SKSE `modList.loadedCCMods[idx]->name`.
 * @returns the mod name if found, empty string if silent API, idx as char* (if A-Z) for test API.
 */

std::optional<std::string_view> loaded_light_mod_name (std::uint8_t idx);

/**
 * Forwards static forms to `SKSESerializationInterface::ResolveHandle`
 * @returns the resolved handle or FormId#Zero on error (or if silent API), same handle if test
 * or the input handle is for dynamic form.
 */

forms::FormId resolve_handle (forms::FormId handle);

/**
 * Valid handles are forwarded to `LookupByFormID`
 * @returns the looked up handle, nullptr if silent API, random blob if test API
 */

TESForm* lookup_form (forms::FormId handle);

/**
 * Uses SKSE `IObjectHandlePolicy::Resolve` and `AddRef`
 * @returns true on successfully retained handle, or if silent/test API
 */

bool try_retain_handle (forms::FormId handle);

/**
 * Forwards to SKSE `IObjectHandlePolicy::Release` (ignored on silent/test API)
 */

void release_handle (forms::FormId handle);

/**
 * If there is a console manager will call its `VPrint` function (ignored on silent/test API).
 */

void console_print (const char * fmt, ...);

/**
 * If there is a console manager will call its `VPrint` function (ignored on silent/test API).
 */

void console_print (const char * fmt, const va_list& args);

/**
 * Binds the skse namespace calls to the real SKSE functions - normal mode for JC.
 */

void set_real_api ();

/**
 * Binds fake/test implementation of all skse calls.
 */

void set_fake_api ();

/**
 * Provides stub implementation of all skse calls - used at the normal JC runtime.
 * @note investigate why
 */

void set_silent_api ();

}

