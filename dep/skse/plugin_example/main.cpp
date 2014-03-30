#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/SafeWrite.h"
#include "skse/ScaleformCallbacks.h"
#include "skse/ScaleformMovie.h"
#include "skse/GameAPI.h"

IDebugLog	gLog("skse_example_plugin.log");

PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

SKSEScaleformInterface		* g_scaleform = NULL;
SKSESerializationInterface	* g_serialization = NULL;

/**** simple gameplay patches ****/

void ApplyPatch(UInt32 base, UInt8 * buf, UInt32 len)
{
	for(UInt32 i = 0; i < len; i++)
		SafeWrite8(base + i, buf[i]);
}

void GameplayPatches(void)
{
	// originally supposed to be 100 /decimal/ but oops
	UInt8	kPickpocketChance[] =
	{
		0xB8, 0x00, 0x01, 0x00, 0x00,	// mov eax, 0x0100
		0xC3							// retn
	};

	ApplyPatch(0x00598DB0, kPickpocketChance, sizeof(kPickpocketChance));

	SafeWrite8(0x008F0850, 0xC3);	// disable achievements
}

/**** scaleform functions ****/

class SKSEScaleform_ExampleFunction : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args)
	{
		Console_Print("hello world from example plugin");
	}
};

bool RegisterScaleform(GFxMovieView * view, GFxValue * root)
{
	RegisterFunction <SKSEScaleform_ExampleFunction>(root, view, "ExampleFunction");

	return true;
}

/**** serialization ****/

void Serialization_Revert(SKSESerializationInterface * intfc)
{
	_MESSAGE("revert");
}

const UInt32 kSerializationDataVersion = 1;

void Serialization_Save(SKSESerializationInterface * intfc)
{
	_MESSAGE("save");

	if(intfc->OpenRecord('DATA', kSerializationDataVersion))
	{
		char	kData[] = "hello world";

		intfc->WriteRecordData(kData, sizeof(kData));
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
			case 'DATA':
			{
				if(version == kSerializationDataVersion)
				{
					if(length)
					{
						char	* buf = new char[length];

						intfc->ReadRecordData(buf, length);
						buf[length - 1] = 0;

						_MESSAGE("read data: %s", buf);
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
	_MESSAGE("skse_example_plugin");

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"example plugin";
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

	// get the scaleform interface and query its version
	g_scaleform = (SKSEScaleformInterface *)skse->QueryInterface(kInterface_Scaleform);
	if(!g_scaleform)
	{
		_MESSAGE("couldn't get scaleform interface");

		return false;
	}

	if(g_scaleform->interfaceVersion < SKSEScaleformInterface::kInterfaceVersion)
	{
		_MESSAGE("scaleform interface too old (%d expected %d)", g_scaleform->interfaceVersion, SKSEScaleformInterface::kInterfaceVersion);

		return false;
	}

	// get the serialization interface and query its version
	g_serialization = (SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization);
	if(!g_serialization)
	{
		_MESSAGE("couldn't get serialization interface");

		return false;
	}

	if(g_serialization->version < SKSESerializationInterface::kVersion)
	{
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

	// apply patches to the game here
	GameplayPatches();

	// register scaleform callbacks
	g_scaleform->Register("example_plugin", RegisterScaleform);

	// register callbacks and unique ID for serialization

	// ### this must be a UNIQUE ID, change this and email me the ID so I can let you know if someone else has already taken it
	g_serialization->SetUniqueID(g_pluginHandle, 'TEST');

	g_serialization->SetRevertCallback(g_pluginHandle, Serialization_Revert);
	g_serialization->SetSaveCallback(g_pluginHandle, Serialization_Save);
	g_serialization->SetLoadCallback(g_pluginHandle, Serialization_Load);

	return true;
}

};
