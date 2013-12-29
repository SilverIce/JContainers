#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include <ShlObj.h>

#include "collections.h"


PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

SKSEScaleformInterface		* g_scaleform = NULL;
SKSESerializationInterface	* g_serialization = NULL;
SKSEPapyrusInterface        * g_papyrus = NULL;

/**** serialization ****/

void Serialization_Revert(SKSESerializationInterface * intfc)
{
	_MESSAGE("revert");
}

const UInt32 kSerializationDataVersion = 1;

enum {
    kJStorageChunk = 'JSTR',
};

void Serialization_Save(SKSESerializationInterface * intfc)
{
	_MESSAGE("save");

	if (intfc->OpenRecord(kJStorageChunk, kSerializationDataVersion)) {
        auto data = collections::shared_state::instance().saveToArray();
        intfc->WriteRecordData(data.data(), data.size());
	}
}

void Serialization_Load(SKSESerializationInterface * intfc)
{
	_MESSAGE("load");

	UInt32	type;
	UInt32	version;
	UInt32	length;
	bool	error = false;

	while(!error && intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch(type)
		{
			case kJStorageChunk:
			{
				if(version == kSerializationDataVersion)
				{
					if(length)
					{
                        std::vector<char> data(length);
                        intfc->ReadRecordData(data.data(), length);
                        collections::shared_state::instance().loadAll(data);
					}
					else
					{
						_MESSAGE("empty data?");
					}
				}
				else
				{
					error = true;
				}
			}
			break;

			default:
				_MESSAGE("unhandled type %08X", type);
				error = true;
				break;
		}
	}
}

extern "C"
{

bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
{
    gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim\\SKSE\\data_structures.log");
    gLog.SetPrintLevel(IDebugLog::kLevel_Error);
    gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

	_MESSAGE("skse_collections ololo");

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"skse collections";
	info->version =		1;

	// store plugin handle so we can identify ourselves later
	g_pluginHandle = skse->GetPluginHandle();

	if(skse->isEditor)
	{
		_MESSAGE("loaded in editor, marking as incompatible");

		return false;
	}
	else if(skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0)
	{
		_MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);

		return false;
	}

    g_papyrus = (SKSEPapyrusInterface *)skse->QueryInterface(kInterface_Papyrus);
    if (!g_papyrus)
    {
        _MESSAGE("couldn't get g_papyrus interface");

        return false;
    }

   
	// get the serialization interface and query its version
	g_serialization = (SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization);
	if(!g_serialization) {
		_MESSAGE("couldn't get serialization interface");
		return false;
	}

	if(g_serialization->version < SKSESerializationInterface::kVersion) {
		_MESSAGE("serialization interface too old (%d expected %d)", g_serialization->version, SKSESerializationInterface::kVersion);
		return false;
	}
    
	// ### do not do anything else in this callback
	// ### only fill out PluginInfo and return true/false

	// supported runtime version
	return true;
}

bool SKSEPlugin_Load(const SKSEInterface * skse)
{
    _MESSAGE("load");

	// register callbacks and unique ID for serialization
	// ### this must be a UNIQUE ID, change this and email me the ID so I can let you know if someone else has already taken it
	g_serialization->SetUniqueID(g_pluginHandle, kJStorageChunk);

	g_serialization->SetRevertCallback(g_pluginHandle, Serialization_Revert);
	g_serialization->SetSaveCallback(g_pluginHandle, Serialization_Save);
	g_serialization->SetLoadCallback(g_pluginHandle, Serialization_Load);

    g_papyrus->Register(collections::tes_object::registerFuncs);
    g_papyrus->Register(collections::tes_array::registerFuncs);
    g_papyrus->Register(collections::tes_map::registerFuncs);

	return true;
}

};



int main(int argc, char** argv) {

    collections::tes_array::registerFuncs(NULL);
    collections::tes_map::registerFuncs(NULL);
     collections::tes_object::registerFuncs(NULL);

    testing::runTests(meta<testing::TestInfo>::getListConst());

    return 0;
}

