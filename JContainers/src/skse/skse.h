#pragma once

#include <stdint.h>
#include "forms/form_id.h"

class TESForm;

// Wraps calls to SKSE. Fakes the calls when SKSE/Skyrim inactive (during synthetic tests)
namespace skse {

    // pass static form ids here only
    const char * modname_from_index(uint8_t idx);
    uint8_t modindex_from_name(const char * name);

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
