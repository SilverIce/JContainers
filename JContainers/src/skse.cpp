#include "skse.h"


//#include <boost/iostreams/stream.hpp>
//#include <ShlObj.h>

//#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameData.h"
//#include "skse/PapyrusVM.h"
#include "skse/GameForms.h"
#include "skse/GameData.h"
#include "skse/InternalSerialization.h"

#include "gtest.h"
//#include "util/util.h"
//#include "jc_interface.h"
//#include "reflection.h"
//#include "jcontainers_constants.h"
//#include "tes_context.h"

namespace skse {

    static bool _is_fake = true;

    namespace fake_skse {

        enum {
            char_count = 'Z' - 'A' + 1,

        };

        static char fakePluginNames[char_count * 2];

        static bool index_in_range(int idx) {
            return idx >= 'A' && idx <= 'Z';
        }

        uint8_t wrap_index(int idx) {
            int rel = (int)idx - (int)'A';
            int wrapped = rel >= 0 ? rel % char_count : (-rel) % char_count;
            assert(wrapped >= 0 && wrapped < char_count);
            return wrapped;
        }

        const char * modname_from_index(uint8_t idx) {

            if (index_in_range(idx)) {
                int wrapped = wrap_index(idx);
                fakePluginNames[wrapped*2] = (char)('A' + wrapped);
                fakePluginNames[wrapped*2 + 1] = '\0';
                return &fakePluginNames[wrapped * 2];
            }
            else {
                return nullptr;
            }
         }

        uint8_t modindex_from_name(const char * name) {
            assert(name);
            return index_in_range(*name) ? *name : 0xFF;
        }

        TEST(modname_from_index, test)
        {
            auto res = modname_from_index('Z');
            EXPECT_TRUE( strcmp(res, "Z") == 0 );

            EXPECT_TRUE(modindex_from_name("Action") == 'A');

            EXPECT_NIL(modname_from_index('|'));
            EXPECT_NIL(modname_from_index('a'));
        }
    }

    uint32_t resolve_handle(uint32_t handle) {
        if (!is_fake()) {
            UInt8	modID = handle >> 24;

            // should not have been saved anyway?
            if (modID == 0xFF)
                return handle;

            UInt8	loadedModID = ResolveModIndex(modID);

            if (loadedModID == 0xFF)
                return handle;

            // fixup ID, success
            handle = (handle & 0x00FFFFFF) | (((uint32_t)loadedModID) << 24);
        }

        return handle;
    }

    const char * modname_from_index(uint8_t idx) {
        if (!is_fake()) {
            DataHandler * dhand = DataHandler::GetSingleton();
            ModInfo * modInfo = idx < _countof(dhand->modList.loadedMods) ? dhand->modList.loadedMods[idx] : nullptr;
            return modInfo ? modInfo->name : nullptr;
        }
        else {
            return fake_skse::modname_from_index(idx);
        }
    }

    uint8_t modindex_from_name(const char * name) {
        return !is_fake() ? DataHandler::GetSingleton()->GetModIndex(name) : fake_skse::modindex_from_name(name);
    }

    // A fake form. Made for imitating SKSE during synthetic tests
    static char fakeTesForm[sizeof TESForm];

    TESForm* lookup_form(uint32_t handle) {
        return !is_fake() ? LookupFormByID(handle) : reinterpret_cast<TESForm*>(&fakeTesForm);
    }

    bool is_fake() {
        return _is_fake;
    }

    void set_no_fake() {
        _is_fake = false;
    }

    void console_print(const char * fmt, const va_list& args) {
        if (is_fake()) {
            return;
        }
        ConsoleManager	* mgr = ConsoleManager::GetSingleton();
        if (mgr) {
            CALL_MEMBER_FN(mgr, Print)(fmt, args);
        }
    }

    void console_print(const char * fmt, ...) {
        va_list	args;
        va_start(args, fmt);
        console_print(fmt, args);
        va_end(args);
    }
}
