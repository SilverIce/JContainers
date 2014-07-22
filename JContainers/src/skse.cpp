
#include <chrono>
#include <exception>

#include <boost/iostreams/stream.hpp>
#include <ShlObj.h>

#include "skse.h"

#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameData.h"
//#include "skse/PapyrusVM.h"
#include "skse/GameForms.h"

#include "gtest.h"
#include "reflection.h"
#include "jcontainers_constants.h"
#include "tes_context.h"

class VMClassRegistry;

namespace collections { namespace {

    static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;

    static SKSESerializationInterface	* g_serialization = NULL;
    static SKSEPapyrusInterface			* g_papyrus = NULL;

    template<class T>
    float do_with_timing(T& func) {
        namespace chr = std::chrono;
        auto started = chr::system_clock::now();

        try {
            func();
        }
        catch (const std::exception& ) {
            assert(false);
        }
        catch (...) {
            assert(false);
        }

        auto ended = chr::system_clock::now();
        return chr::duration_cast<chr::milliseconds>(ended - started).count() / 1000.f;
    }

    void revert(SKSESerializationInterface * intfc) {
        _DMESSAGE("Revert started");
        float diff = do_with_timing([]() {
            collections::tes_context::instance().clearState();
        });
        _DMESSAGE("Revert finished in %f sec", diff);
    }

    void save(SKSESerializationInterface * intfc) {

        namespace io = boost::iostreams;

        struct skse_data_sink {
            typedef char      char_type;
            typedef io::sink_tag  category;

            std::streamsize write(const char* s, std::streamsize n) const {
                (void)_sink->WriteRecordData(s, n); // always returns true
                return n;
            }

            SKSESerializationInterface* _sink;
        };


        float diff = do_with_timing([intfc]() {
            _DMESSAGE("Save started");

            if (intfc->OpenRecord(kJStorageChunk, kJSerializationCurrentVersion)) {
                io::stream<skse_data_sink> stream(skse_data_sink{ intfc });
                collections::tes_context::instance().write_to_stream(stream);
                //_DMESSAGE("%lu bytes saved", stream.tellp());
            }
            else {
                _DMESSAGE("Unable open JC record");
            }
        });

        _DMESSAGE("Save finished in %f sec", diff);
    }

    void load(SKSESerializationInterface * intfc) {

        namespace io = boost::iostreams;

        class skse_data_source {
        public:
            typedef char char_type;
            typedef io::source_tag  category;

            explicit skse_data_source(SKSESerializationInterface* src = nullptr) : _source(src){}

            std::streamsize read(char* buffer, std::streamsize n) const {
                return _source ? _source->ReadRecordData(buffer, n) : 0;
            }

        private:
            SKSESerializationInterface* _source;
        };

        float diff = do_with_timing([intfc]() {

            _DMESSAGE("Load started");

            UInt32 type = 0;
            UInt32 version = 0;
            UInt32 length = 0;

            while (intfc->GetNextRecordInfo(&type, &version, &length)) {
                if (type == kJStorageChunk) {
                    break;
                }
            }

            io::stream<skse_data_source> stream(skse_data_source(type == kJStorageChunk ? intfc : nullptr));
            collections::tes_context::instance().read_from_stream(stream, version);
        });

        _DMESSAGE("Load finished in %f sec", diff);
    }

    extern "C" {

        __declspec(dllexport) bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
        {
            gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim\\SKSE\\JContainers.log");
            gLog.SetPrintLevel(IDebugLog::kLevel_Error);
            gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

            // populate info structure
            info->infoVersion = PluginInfo::kInfoVersion;
            info->name = "JContainers";
            info->version = 1;

            // store plugin handle so we can identify ourselves later
            g_pluginHandle = skse->GetPluginHandle();

            _MESSAGE("JContainers %u.%u.%u\n", kJVersionMajor, kJVersionMinor, kJVersionPatch);

            if (skse->isEditor) {
                _MESSAGE("loaded in editor, marking as incompatible");
                return false;
            }
            else if (skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0) {
                _MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);
                return false;
            }

            // get the serialization interface and query its version
            g_serialization = (SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization);
            if (!g_serialization) {
                _MESSAGE("couldn't get serialization interface");
                return false;
            }

            if (g_serialization->version < SKSESerializationInterface::kVersion) {
                _MESSAGE("serialization interface too old (%d expected %d)", g_serialization->version, SKSESerializationInterface::kVersion);
                return false;
            }

            g_papyrus = (SKSEPapyrusInterface *)skse->QueryInterface(kInterface_Papyrus);

            if (!g_papyrus) {
                _MESSAGE("couldn't get papyrus interface");
                return false;
            }

            return true;
        }

        bool registerAllFunctions(VMClassRegistry *registry) {

            _MESSAGE("registering functions");

            reflection::foreach_metaInfo_do([=](const reflection::class_info& info) {
                info.bind(registry);
            });

            return true;
        }

        __declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface * skse) {

            g_serialization->SetUniqueID(g_pluginHandle, kJStorageChunk);

            g_serialization->SetRevertCallback(g_pluginHandle, revert);
            g_serialization->SetSaveCallback(g_pluginHandle, save);
            g_serialization->SetLoadCallback(g_pluginHandle, load);

            g_papyrus->Register(registerAllFunctions);

            _MESSAGE("plugin loaded");

            return true;
        }

        __declspec(dllexport) void launchShityTest() {
            testing::runTests(meta<testing::TestInfo>::getListConst());
        }
    };

}

}

namespace collections { namespace skse {

    namespace aux {

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
        if (g_serialization) {
            UInt64 newId = handle;
            g_serialization->ResolveHandle(handle, &newId);
            return newId;
        }

        return handle;
    }

    const char * modname_from_index(uint8_t idx) {
        if (g_serialization) {
            DataHandler * dhand = DataHandler::GetSingleton();
            ModInfo * modInfo = idx < _countof(dhand->modList.loadedMods) ? dhand->modList.loadedMods[idx] : nullptr;
            return modInfo ? modInfo->name : nullptr;
        }
        else {
            return aux::modname_from_index(idx);
        }
    }

    uint8_t modindex_from_name(const char * name) {
        return g_serialization ? DataHandler::GetSingleton()->GetModIndex(name) : aux::modindex_from_name(name);
    }

    TESForm* lookup_form(uint32_t handle) {
        return g_serialization ? LookupFormByID(handle) : nullptr;
    }
}
}
