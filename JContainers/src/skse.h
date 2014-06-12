#pragma once

#include <stdint.h>

class TESForm;

namespace skse {
    const char * modname_from_index(uint8_t idx);
    uint8_t modindex_from_name(const char * name);

	extern const char * fake_plugin_name;
	const uint8_t fake_plugin_index = 0;

    uint32_t resolve_handle(uint32_t handle);
    TESForm* lookup_form(uint32_t handle);
}

