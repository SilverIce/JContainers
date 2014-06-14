#pragma once

#include <stdint.h>

class TESForm;

namespace skse {
    // pass static form ids here only
    const char * modname_from_index(uint8_t idx);
    uint8_t modindex_from_name(const char * name);

    uint32_t resolve_handle(uint32_t handle);
    TESForm* lookup_form(uint32_t handle);
}

