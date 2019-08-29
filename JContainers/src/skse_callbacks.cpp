#include <boost/iostreams/stream.hpp>
#include <ShlObj.h>

#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameData.h"
//#include "skse/PapyrusVM.h"
#include "skse/GameForms.h"
#include "skse/GameData.h"

//#include "gtest.h"
#include "skse/skse.h"
#include "skse/PapyrusVM.h"
#include "util/util.h"
#include "jc_interface.h"
#include "reflection/reflection.h"
#include "jcontainers_constants.h"

#include "collections/context.h"
#include "forms/form_observer.h"

#include "domains/domain_master.h"

class VMClassRegistry;

namespace jc {
    extern root_interface root;
}

SKSESerializationInterface	* g_serialization = nullptr;
static SKSEPapyrusInterface			* g_papyrus = nullptr;
static SKSEMessagingInterface       * g_messaging = nullptr;

namespace {

    using namespace collections;
    using namespace domain_master;

    static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;

    void revert(SKSESerializationInterface * intfc) {
        util::do_with_timing("Revert", []() {
            skse::set_silent_api();
            domain_master::master::instance().clear_state();
            skse::set_real_api();
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
                domain_master::master::instance().write_to_stream(stream);
                //_DMESSAGE("%lu bytes saved", stream.tellp());
            }
            else {
                JC_log("Unable open JC record");
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

            skse::set_silent_api();
            domain_master::master::instance().clear_state();
            skse::set_real_api();

            UInt32 type = 0;
            UInt32 version = 0;
            UInt32 length = 0;

            while (intfc->GetNextRecordInfo(&type, &version, &length)) {
                if (static_cast<consts>(type) == consts::storage_chunk) {
                    break;
                }
            }

            io::stream<skse_data_source> stream(skse_data_source(static_cast<consts>(type) == consts::storage_chunk ? intfc : nullptr));
            domain_master::master::instance().read_from_stream(stream);
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

            JC_log(JC_PLUGIN_NAME " " JC_VERSION_STR);

            if (skse->isEditor) {
                JC_log("loaded in editor, marking as incompatible");
                return false;
            }
            else if (skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0) {
                JC_log("unsupported runtime version %08X", skse->runtimeVersion);
                return false;
            }

            // get the serialization interface and query its version
            g_serialization = (SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization);
            if (!g_serialization) {
                JC_log("couldn't get serialization interface");
                return false;
            }

            if (g_serialization->version < SKSESerializationInterface::kVersion) {
                JC_log("serialization interface too old (%d expected %d)", g_serialization->version, SKSESerializationInterface::kVersion);
                return false;
            }

            g_papyrus = (SKSEPapyrusInterface *)skse->QueryInterface(kInterface_Papyrus);

            if (!g_papyrus) {
                JC_log("couldn't get papyrus interface");
                return false;
            }

            auto messaging = (SKSEMessagingInterface *)skse->QueryInterface(kInterface_Messaging);
            if (messaging && messaging->interfaceVersion >= SKSEMessagingInterface::kInterfaceVersion) {
                g_messaging = messaging;
            }

            skse::set_real_api();

            return true;
        }

        bool registerAllFunctions(VMClassRegistry *registry) {

            jc_assert(registry);

            // One of the ways: temp. clone class meta infos, register them
            // 2nd: pass each context into "info.bind(*registry, some-context);"

            // Anyway, in a result we must construct plenty of SKSE native functors, each function will store a pointer
            // to tes_context instance. The functions will be registered in Papyrus VM

            // context_master should return tes_context's for us
            // Here we should iterator over files in JCData/Contexts/ folder:
            // for f in "./JCData/Contexts/*.json":
            //   c = master.get_context(f)
            //   for cls in metainfo:
            //     cls.register(c,f)

            // Need to enhance control over resulting function and class name
            // E.g. turn JArray.addObj into PSM_JContainers.JArray_addObj

            // Pitfall: since the functions registered only ONCE, we must 
            // preserve context pointers during ALL gaming session

            // ����� ��������, ��� ����� ���� ���� ����� �������� ���� �� ��������

            util::do_with_timing("Registering functions", [=]() {

                auto& master = domain_master::master::instance();

                auto registerDom = [&](const reflection::class_info& info, domain_master::context& dom) {
                    info.visit_functions([&](const reflection::function_info& func) {
                        if (&dom != &master.get_default_domain() && func.isStateless()) {
                            return;
                        }
                        reflection::bind_args args{
                            *registry,
                            info.className(),
                            func.name,
                            reinterpret_cast<reflection::bind_args::shared_state_t*>(&dom)
                        };
                        func.registrator(args);
                        registry->SetFunctionFlags(args.className.c_str(),
                            args.functionName.c_str(), VMClassRegistry::kFunctionFlag_NoWait);
                    });
                };

                reflection::foreach_metaInfo_do(reflection::class_registry(), registerDom, master.get_default_domain());

                reflection::class_info amalgam = reflection::amalgamate_classes("dummy", reflection::class_registry());

                for (auto& domName : master.active_domain_names) {
                    auto& dom = master.get_or_create_domain_with_name(domName);
                    amalgam._className = domName;
                    registerDom(amalgam, dom);
                }
            });

            util::do_with_timing("Publising C API", [=]() {
                if (g_messaging) {
                    g_messaging->Dispatch(g_pluginHandle, jc::message_root_interface, (void*)&jc::root, sizeof(void*), nullptr);
                }
            });

            return true;
        }

        __declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface * skse) {

            g_serialization->SetUniqueID(g_pluginHandle, (UInt32)consts::storage_chunk);

            g_serialization->SetRevertCallback(g_pluginHandle, revert);
            g_serialization->SetSaveCallback(g_pluginHandle, save);
            g_serialization->SetLoadCallback(g_pluginHandle, load);

            g_serialization->SetFormDeleteCallback(g_pluginHandle, [](UInt64 handle) {
                domain_master::master::instance().get_form_observer().on_form_deleted((forms::FormHandle)handle);
            });

            g_papyrus->Register(registerAllFunctions);

            JC_log("plugin loaded");

            return true;
        }
    };

}


