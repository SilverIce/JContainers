#pragma once

#include <stdint.h>

class TESForm;

// Wraps calls to SKSE. Fakes the calls when SKSE/Skyrim inactive (during synthetic tests)
namespace skse {
    // pass static form ids here only
    const char * modname_from_index(uint8_t idx);
    uint8_t modindex_from_name(const char * name);

    uint32_t resolve_handle(uint32_t handle);
    TESForm* lookup_form(uint32_t handle);

    bool is_fake();
    void set_no_fake();
    void console_print(const char * fmt, ...);
    void console_print(const char * fmt, const va_list& args);
}
