#pragma once

#include <stdint.h>
#include "form_id.h"

class TESForm;

// Wraps calls to SKSE. Fakes the calls when SKSE/Skyrim inactive (during synthetic tests)
namespace skse {

    namespace {
        using collections::FormId;
        using collections::FormIdUnredlying;
    }

    // pass static form ids here only
    const char * modname_from_index(uint8_t idx);
    uint8_t modindex_from_name(const char * name);

    FormId resolve_handle(FormId handle);
    TESForm* lookup_form(FormId handle);

    void retain_handle(FormId handle);
    void release_handle(FormId handle);

    bool is_fake();
    void set_no_fake();
    void console_print(const char * fmt, ...);
    void console_print(const char * fmt, const va_list& args);
}
