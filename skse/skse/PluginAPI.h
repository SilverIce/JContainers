#pragma once

typedef UInt32	PluginHandle;	// treat this as an opaque type
class GFxMovieView;
class GFxValue;
class TaskDelegate;

enum
{
	kPluginHandle_Invalid = 0xFFFFFFFF
};

enum
{
	kInterface_Invalid = 0,
	kInterface_Scaleform,
	kInterface_Papyrus,
	kInterface_Serialization,
	kInterface_Task,

	kInterface_Max,
};

struct SKSEInterface
{
	UInt32	skseVersion;
	UInt32	runtimeVersion;
	UInt32	editorVersion;
	UInt32	isEditor;
	void *	(* QueryInterface)(UInt32 id);

	// call during your Query or Load functions to get a PluginHandle uniquely identifying your plugin
	// invalid if called at any other time, so call it once and save the result
	PluginHandle	(* GetPluginHandle)(void);
};

struct SKSEScaleformInterface
{
	enum
	{
		kInterfaceVersion = 1
	};

	UInt32	interfaceVersion;

	// This callback will be called once for every new menu that is created.
	// Create your objects relative to the 'root' GFxValue parameter.
	typedef bool (* RegisterCallback)(GFxMovieView * view, GFxValue * root);

	// Register your plugin's scaleform API creation callback here.
	// The "name" parameter will be used to create an object with the path:
	// "skse.plugins.name" that will be passed to the callback.
	// Make sure that the memory it points to is valid from the point the callback
	// is registered until the game exits.
	bool	(* Register)(const char * name, RegisterCallback callback);
};

struct SKSESerializationInterface
{
	enum
	{
		kVersion = 2,
	};

	typedef void (* EventCallback)(SKSESerializationInterface * intfc);

	typedef void (* FormDeleteCallback)(UInt64 handle);

	UInt32	version;

	void	(* SetUniqueID)(PluginHandle plugin, UInt32 uid);

	void	(* SetRevertCallback)(PluginHandle plugin, EventCallback callback);
	void	(* SetSaveCallback)(PluginHandle plugin, EventCallback callback);
	void	(* SetLoadCallback)(PluginHandle plugin, EventCallback callback);
	void	(* SetFormDeleteCallback)(PluginHandle plugin, FormDeleteCallback callback);

	bool	(* WriteRecord)(UInt32 type, UInt32 version, const void * buf, UInt32 length);
	bool	(* OpenRecord)(UInt32 type, UInt32 version);
	bool	(* WriteRecordData)(const void * buf, UInt32 length);

	bool	(* GetNextRecordInfo)(UInt32 * type, UInt32 * version, UInt32 * length);
	UInt32	(* ReadRecordData)(void * buf, UInt32 length);
	bool	(* ResolveHandle)(UInt64 handle, UInt64 * handleOut);
};

struct SKSETaskInterface
{
	enum
	{
		kInterfaceVersion = 1
	};

	UInt32	interfaceVersion;

	// Derive your type from TaskDelegate
	// Allocate before adding
	// Define your Run function
	// Delete your object in the Dispose call
	void	(* AddTask)(TaskDelegate * task);
};

#ifdef _PPAPI

// ### this code is unsupported and will be changed in the future

class VMClassRegistry;

struct SKSEPapyrusInterface
{
	enum
	{
		kInterfaceVersion = 1
	};
	UInt32	interfaceVersion;
	typedef bool (* RegisterFunctions)(VMClassRegistry * registry);
	bool	(* Register)(RegisterFunctions callback);
};

#endif

struct PluginInfo
{
	enum
	{
		kInfoVersion = 1
	};

	UInt32			infoVersion;
	const char *	name;
	UInt32			version;
};

typedef bool (* _SKSEPlugin_Query)(const SKSEInterface * skse, PluginInfo * info);
typedef bool (* _SKSEPlugin_Load)(const SKSEInterface * skse);

/**** plugin API docs **********************************************************
 *	
 *	The base API is pretty simple. Create a project based on the
 *	skse_plugin_example project included with the SKSE source code, then define
 *	and export these functions:
 *	
 *	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
 *	
 *	This primary purposes of this function are to fill out the PluginInfo
 *	structure, and to perform basic version checks based on the info in the
 *	SKSEInterface structure. Return false if your plugin is incompatible with
 *	the version of SKSE or the runtime passed in, otherwise return true. In
 *	either case, fill out the PluginInfo structure.
 *	
 *	Do not do anything other than fill out the PluginInfo structure and return
 *	true/false in this callback.
 *	
 *	If the plugin is being loaded in the context of the editor, isEditor will be
 *	non-zero, editorVersion will contain the current editor version, and
 *	runtimeVersion will be zero. In this case you can probably just return
 *	true, however if you have multiple DLLs implementing the same behavior, for
 *	example one for each version of ther runtime, only one of them should return
 *	true.
 *	
 *	The PluginInfo fields should be filled out as follows:
 *	- infoVersion should be set to PluginInfo::kInfoVersion
 *	- name should be a pointer to a null-terminated string uniquely identifying
 *	  your plugin, it will be used in the plugin querying API
 *	- version is only used by the plugin query API, and will be returned to
 *	  scripts requesting the current version of your plugin
 *	
 *	bool SKSEPlugin_Load(const SKSEInterface * skse)
 *	
 *	In this function, use the interfaces above to register your commands, patch
 *	memory, generally do whatever you need to for integration with the runtime.
 *	
 *	At this time, or at any point forward you can call the QueryInterface
 *	callback to retrieve an interface structure for the base services provided
 *	by the SKSE core.
 *	
 *	You may optionally return false from this function to unload your plugin,
 *	but make sure that you DO NOT register any commands if you do.
 *	
 *	Note that all structure versions are backwards-compatible, so you only need
 *	to check against the latest version that you need. New fields will be only
 *	added to the end, and all old fields will remain compatible with their
 *	previous implementations.
 *	
 ******************************************************************************/
