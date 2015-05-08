#include <boost/iostreams/stream.hpp>
#include <ShlObj.h>

#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameData.h"
//#include "skse/PapyrusVM.h"
#include "skse/GameForms.h"
#include "skse/GameData.h"

//#include "gtest.h"
#include "skse.h"
#include "util/util.h"
#include "jc_interface.h"
#include "reflection.h"
#include "jcontainers_constants.h"
#include "tes_context.h"

class VMClassRegistry;

namespace jc {
    extern root_interface root;
}

namespace {

    using namespace collections;

    static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;

    static SKSESerializationInterface	* g_serialization = nullptr;
    static SKSEPapyrusInterface			* g_papyrus = nullptr;
    static SKSEMessagingInterface       * g_messaging = nullptr;


    void revert(SKSESerializationInterface * intfc) {
        util::do_with_timing("Revert", []() {
            collections::tes_context::instance().clearState();
        });
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


        util::do_with_timing("Save", [intfc]() {
            if (intfc->OpenRecord((UInt32)consts::storage_chunk, (UInt32)serialization_version::current)) {
                io::stream<skse_data_sink> stream(skse_data_sink{ intfc });
                collections::tes_context::instance().write_to_stream(stream);
                //_DMESSAGE("%lu bytes saved", stream.tellp());
            }
            else {
                _DMESSAGE("Unable open JC record");
            }
        });
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

        util::do_with_timing("Load", [intfc]() {

            UInt32 type = 0;
            UInt32 version = 0;
            UInt32 length = 0;

            while (intfc->GetNextRecordInfo(&type, &version, &length)) {
                if (static_cast<consts>(type) == consts::storage_chunk) {
                    break;
                }
            }

            io::stream<skse_data_source> stream(skse_data_source(static_cast<consts>(type) == consts::storage_chunk ? intfc : nullptr));
            collections::tes_context::instance().read_from_stream(stream, (serialization_version)version);
        });
    }

    extern "C" {

        __declspec(dllexport) bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info) {
            gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim\\SKSE\\" JC_PLUGIN_NAME ".log");
            gLog.SetPrintLevel(IDebugLog::kLevel_Error);
            gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

            // populate info structure
            info->infoVersion = PluginInfo::kInfoVersion;
            info->name = JC_PLUGIN_NAME;
            info->version = JC_API_VERSION;

            // store plugin handle so we can identify ourselves later
            g_pluginHandle = skse->GetPluginHandle();

            _MESSAGE(JC_PLUGIN_NAME " " JC_VERSION_STR);

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

            auto messaging = (SKSEMessagingInterface *)skse->QueryInterface(kInterface_Messaging);
            if (messaging && messaging->interfaceVersion >= SKSEMessagingInterface::kInterfaceVersion) {
                g_messaging = messaging;
            }

            skse::set_no_fake();

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

            g_serialization->SetUniqueID(g_pluginHandle, (UInt32)consts::storage_chunk);

            g_serialization->SetRevertCallback(g_pluginHandle, revert);
            g_serialization->SetSaveCallback(g_pluginHandle, save);
            g_serialization->SetLoadCallback(g_pluginHandle, load);

            g_papyrus->Register(registerAllFunctions);

            if (g_messaging) {
                g_messaging->RegisterListener(g_pluginHandle, "SKSE", [](SKSEMessagingInterface::Message* msg) {
                    if (msg && msg->type == SKSEMessagingInterface::kMessage_PostPostLoad) {
                        g_messaging->Dispatch(g_pluginHandle, jc::message_root_interface, (void *)&jc::root, sizeof(void*), nullptr);
                    }
                });
            }

            _MESSAGE("plugin loaded");

            return true;
        }
    };

}


