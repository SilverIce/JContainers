#include "skse/skse.h"


//#include <boost/iostreams/stream.hpp>
//#include <ShlObj.h>

//#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameData.h"
//#include "skse/PapyrusVM.h"
#include "skse/GameForms.h"
#include "skse/GameData.h"
#include "skse/InternalSerialization.h"
#include "skse/PluginAPI.h"
#include "skse/PapyrusVM.h"

#include "util/stl_ext.h"
#include "gtest.h"

#include "collections/form_handling.h"
//#include "util/util.h"
//#include "jc_interface.h"
//#include "reflection/reflection.h"
//#include "jcontainers_constants.h"
//#include "collections/tes_context.h"

extern SKSESerializationInterface	* g_serialization;

namespace skse {

    namespace {
        struct skse_api {
            virtual const char * modname_from_index(uint8_t idx) = 0;
            virtual uint8_t modindex_from_name(const char * name) = 0;

            virtual FormId resolve_handle(FormId handle) = 0;
            virtual TESForm* lookup_form(FormId handle) = 0;

            virtual bool try_retain_handle(FormId handle) = 0;
            virtual void release_handle(FormId handle) = 0;

            virtual void console_print(const char * fmt, const va_list& args) = 0;
        };

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
                    fakePluginNames[wrapped * 2] = (char)('A' + wrapped);
                    fakePluginNames[wrapped * 2 + 1] = '\0';
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
                EXPECT_TRUE(strcmp(res, "Z") == 0);

                EXPECT_TRUE(modindex_from_name("Action") == 'A');

                EXPECT_NIL(modname_from_index('|'));
                EXPECT_NIL(modname_from_index('a'));
            }
        }

        struct skse_fake_api : skse_api {
            const char * modname_from_index(uint8_t idx) override {
                return fake_skse::modname_from_index(idx);
            }

            uint8_t modindex_from_name(const char * name) override {
                return fake_skse::modindex_from_name(name);
            }

            FormId resolve_handle(FormId handle) override {
                return handle;
            }

            TESForm* lookup_form(FormId handle) override {
                // Just a blob of bytes which imitates TESForm
                static char fakeTesForm[sizeof TESForm] = { '\0' };
                return reinterpret_cast<TESForm*>(&fakeTesForm);
            }

            bool try_retain_handle(FormId handle) override { return true; }
            void release_handle(FormId handle) override {}

            void console_print(const char * fmt, const va_list& args) override {
/*
                printf("Fake Console: ");
                vprintf_s(fmt, args);
                printf("\n");*/
            }
        };

        struct skse_silent_api : skse_api {
            const char * modname_from_index(uint8_t idx) override { return ""; }
            uint8_t modindex_from_name(const char * name) override { return 0; }
            FormId resolve_handle(FormId handle) override { return FormId::Zero; }
            TESForm* lookup_form(FormId handle) override { return nullptr; }
            bool try_retain_handle(FormId handle) override { return true; }
            void release_handle(FormId handle) override {}
            void console_print(const char * fmt, const va_list& args) override {}
        };

        struct skse_real_api : skse_api {
            const char * modname_from_index(uint8_t idx) override {
                DataHandler * dhand = DataHandler::GetSingleton();
                ModInfo * modInfo = idx < _countof(dhand->modList.loadedMods) ? dhand->modList.loadedMods[idx] : nullptr;
                return modInfo ? modInfo->name : nullptr;
            }

            uint8_t modindex_from_name(const char * name) override {
                return DataHandler::GetSingleton()->GetModIndex(name);
            }

            FormId resolve_handle(FormId handle) override {
                if ((util::to_integral(handle) >> 24) == 0xFF) { // dynamic form - return untouched
                    return handle;
                }

                UInt64 handleOut = 0;
                return g_serialization->ResolveHandle(util::to_integral(handle), &handleOut) ? (FormId)handleOut : FormId::Zero;
            }

            TESForm* lookup_form(FormId handle) override {
                return LookupFormByID(util::to_integral(handle));
            }

            bool try_retain_handle(FormId id) override {
                auto handle = util::to_integral(collections::form_handling::form_id_to_handle(id));
                if ((*g_objectHandlePolicy)->Resolve(TESForm::kTypeID, handle) != nullptr) {
                    (*g_objectHandlePolicy)->AddRef(handle);
                    return true;
                }
                return false;
            }
            void release_handle(FormId handle) override {
                (*g_objectHandlePolicy)->Release(util::to_integral(collections::form_handling::form_id_to_handle(handle)));
            }

            void console_print(const char * fmt, const va_list& args) override {
                ConsoleManager	* mgr = ConsoleManager::GetSingleton();
                if (mgr) {
                    CALL_MEMBER_FN(mgr, Print)(fmt, args);
                }
            }
        };

        skse_fake_api g_fake_api;
        skse_real_api g_real_api;
        skse_silent_api g_silent_api;

        skse_api* g_current_api = &g_fake_api;
    }


    void set_real_api() {
        g_current_api = &g_real_api;
    }
    void set_fake_api() {
        g_current_api = &g_fake_api;
    }
    void set_silent_api() {
        g_current_api = &g_silent_api;
    }

    FormId resolve_handle(FormId handle) {
        return g_current_api->resolve_handle(handle);
    }

    TESForm* lookup_form(FormId handle) {
        return g_current_api->lookup_form(handle);
    }

    const char * modname_from_index(uint8_t idx) {
        return g_current_api->modname_from_index(idx);
    }

    uint8_t modindex_from_name(const char * name) {
        return g_current_api->modindex_from_name(name);
    }

    void console_print(const char * fmt, const va_list& args) {
        g_current_api->console_print(fmt, args);
    }

    void console_print(const char * fmt, ...) {
        va_list	args;
        va_start(args, fmt);
        console_print(fmt, args);
        va_end(args);
    }

    bool try_retain_handle(FormId handle) {
        return g_current_api->try_retain_handle(handle);
    }

    void release_handle(FormId handle) {
        g_current_api->release_handle(handle);
    }

}
