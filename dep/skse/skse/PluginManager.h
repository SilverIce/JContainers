#pragma once

#include <string>
#include <vector>

#include "skse/PluginAPI.h"

class PluginManager
{
public:
	PluginManager();
	~PluginManager();

	bool	Init(void);
	void	DeInit(void);

	PluginInfo *	GetInfoByName(const char * name);

	UInt32			GetNumPlugins(void);

	static void *		QueryInterface(UInt32 id);
	static PluginHandle	GetPluginHandle(void);

private:
	struct LoadedPlugin
	{
		// internals
		HMODULE		handle;
		PluginInfo	info;

		_SKSEPlugin_Query	query;
		_SKSEPlugin_Load	load;

		// scaleform info
		SKSEScaleformInterface::RegisterCallback	scaleformRegisterCallback;
		const char	* scaleformName;
	};

	bool	FindPluginDirectory(void);
	void	InstallPlugins(void);

	const char *	SafeCallQueryPlugin(LoadedPlugin * plugin, const SKSEInterface * skse);
	const char *	SafeCallLoadPlugin(LoadedPlugin * plugin, const SKSEInterface * skse);

	const char *	CheckPluginCompatibility(LoadedPlugin * plugin);

	typedef std::vector <LoadedPlugin>	LoadedPluginList;

	std::string			m_pluginDirectory;
	LoadedPluginList	m_plugins;

	static LoadedPlugin		* s_currentLoadingPlugin;
	static PluginHandle		s_currentPluginHandle;
};

extern PluginManager	g_pluginManager;
