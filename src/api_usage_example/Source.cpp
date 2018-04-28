/*
    Primitive example which shows how to use SKSE messaging API and interact with JContainers API

    Plugin obtains some JC functionality and registers a function (sortByName) which sorts an jarray of forms by their names
*/


#include "common/IPrefix.h"

#include <ShlObj.h>
#include <assert.h>

#include "jc_interface.h"

#include "skse64/PluginAPI.h"
#include "skse64_common/skse_version.h"
#include "skse64/GameForms.h"
#include "skse64/PapyrusNativeFunctions.h"
#include "skse64/PapyrusForm.h"

class VMClassRegistry;


static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
static SKSEPapyrusInterface			* g_papyrus = NULL;
static SKSEMessagingInterface       *g_messaging = nullptr;

#define PLUGIN_NAME "JC_API_Example"


namespace {

    //
    void * default_domain = nullptr;

    // function pointers which will be obtained:
    SInt32(*JArray_size)(void*, SInt32 obj) = nullptr;
    TESForm* (*JArray_getForm)(void*, SInt32 obj, SInt32 idx, TESForm* def) = nullptr;
    void (*JArray_swap)(void*, SInt32 obj, SInt32 idx, SInt32 idx2) = nullptr;

    template<class T>
    void obtain_func(const jc::reflection_interface *refl, const char *funcName, const char *className, T& func) {
        assert(refl);
        func = (T)refl->tes_function_of_class(funcName, className);
        assert(func);
    }

    void OnJCAPIAvailable(const jc::root_interface * root) {

        _MESSAGE("OnJCAPIAvailable");

        // Current API is not very usable - you'll have to obtain functions manually:

        auto refl = root->query_interface<jc::reflection_interface>();

        obtain_func(refl, "count", "JArray", JArray_size);
        obtain_func(refl, "getForm", "JArray", JArray_getForm);
        obtain_func(refl, "swapItems", "JArray", JArray_swap);

        default_domain = root->query_interface<jc::domain_interface>()->get_default_domain();
    }

    void sortByName(StaticFunctionTag*, SInt32 obj) {

        auto array_size = JArray_size(default_domain, obj);

        // >
        auto compare = [obj](int idx, int idx2) {
            auto n1 = papyrusForm::GetName(JArray_getForm(default_domain, obj, idx, nullptr));
            auto n2 = papyrusForm::GetName(JArray_getForm(default_domain, obj, idx2, nullptr));

            return n1.data && n2.data && _stricmp(n1.data, n2.data) > 0;
        };

        //Bubble Sorting begins
        for (int passes = 0; passes < array_size - 1; passes++) {
            for (int j = 0; j < array_size - passes - 1; j++) {
                if (compare(j, j + 1)) {
                    JArray_swap(default_domain, obj, j, j + 1);
                }
            }
        }  //Bubble Sorting finished
    }

    bool registerAllFunctions(VMClassRegistry *registry) {

        auto funcName = "sortByName";
        auto className = PLUGIN_NAME;

        registry->RegisterFunction(
            new NativeFunction1 <StaticFunctionTag, void, SInt32>(funcName, className, sortByName, registry));

        registry->SetFunctionFlags(className, funcName, VMClassRegistry::kFunctionFlag_NoWait);

        _MESSAGE("registering functions");

        return true;
    }
}

extern "C" {

    __declspec(dllexport) bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
    {
        gLog.OpenRelative (CSIDL_MYDOCUMENTS, JC_SKSE_LOGS PLUGIN_NAME ".log");
        gLog.SetPrintLevel (IDebugLog::kLevel_Error);
        gLog.SetLogLevel (IDebugLog::kLevel_DebugMessage);

        // populate info structure
        info->infoVersion = PluginInfo::kInfoVersion;
        info->name = PLUGIN_NAME;
        info->version = 1;

        // store plugin handle so we can identify ourselves later
        g_pluginHandle = skse->GetPluginHandle();

        if (skse->isEditor) {
            _MESSAGE("loaded in editor, marking as incompatible");
            return false;
        }
        else if (skse->runtimeVersion != CURRENT_RELEASE_RUNTIME) {
            _MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);
            return false;
        }

        g_papyrus = (SKSEPapyrusInterface *)skse->QueryInterface(kInterface_Papyrus);

        if (!g_papyrus) {
            _MESSAGE("couldn't get papyrus interface");
            return false;
        }

        g_messaging = (SKSEMessagingInterface *)skse->QueryInterface(kInterface_Messaging);

        if (!g_messaging) {
            _MESSAGE("messaging api not found");
            return false;
        }

        return true;
    }

    __declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface * skse) {

        g_papyrus->Register(registerAllFunctions);

        g_messaging->RegisterListener(g_pluginHandle, "SKSE", [](SKSEMessagingInterface::Message* msg) {

            if (msg && msg->type == SKSEMessagingInterface::kMessage_PostLoad) {

                auto loaded = g_messaging->RegisterListener(g_pluginHandle, "JContainers64", [](SKSEMessagingInterface::Message* msg) {

                    // JC publishes its API:
                    if (msg && msg->type == jc::message_root_interface) {
                        OnJCAPIAvailable(jc::root_interface::from_void(msg->data));
                    }
                });
            }
        });

        _MESSAGE("plugin loaded");

        return true;
    }
}
