#include <tlhelp32.h>
#include <direct.h>
#include <shlobj.h>
#include <string>
#include "common/IFileStream.h"
#include "loader_common/IdentifyEXE.h"
#include "loader_common/Error.h"
#include "Options.h"
#include "Inject.h"
#include "Steam.h"
#include "skse/Utilities.h"
#include "skse/skse_version.h"

IDebugLog	gLog;

static bool InjectDLL(PROCESS_INFORMATION * info, const char * dllPath, ProcHookInfo * hookInfo);
static bool InjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync);
static bool DoInjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync);
static void PrintModuleInfo(UInt32 procID);

int main(int argc, char ** argv)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim\\SKSE\\skse_loader.log");
	gLog.SetPrintLevel(IDebugLog::kLevel_FatalError);
	gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

	FILETIME	now;
	GetSystemTimeAsFileTime(&now);

	_MESSAGE("skse loader %08X %08X%08X %s",
		PACKED_SKSE_VERSION, now.dwHighDateTime, now.dwLowDateTime, GetOSInfoStr().c_str());

	if(!g_options.Read(argc, argv))
	{
		PrintLoaderError("Couldn't read arguments.");
		g_options.PrintUsage();

		return 1;
	}

	if(g_options.m_optionsOnly)
	{
		g_options.PrintUsage();
		return 0;
	}

	if(g_options.m_verbose)
		gLog.SetPrintLevel(IDebugLog::kLevel_VerboseMessage);

	if(g_options.m_launchCS)
		_MESSAGE("launching editor");

	// get process/dll names
	bool		dllHasFullPath = false;
	const char	* baseDllName = g_options.m_launchCS ? "skse_editor" : "skse";
	bool		usedCustomRuntimeName = false;

	std::string	procName;

	if(g_options.m_launchCS)
	{
		procName = "CreationKit.exe";
	}
	else
	{
		procName = GetConfigOption("Loader", "RuntimeName");
		if(!procName.empty())
		{
			_MESSAGE("using runtime name from config: %s", procName.c_str());
			usedCustomRuntimeName = true;
		}
		else
		{
			procName = "TESV.exe";

			// simple check to see if someone kludge-patched the EXE
			// don't kludge the EXE, use the .ini file RIGHT ABOVE HERE
			UInt32	procNameCheck =
				(procName[0] <<  8) |
				(procName[1] << 24) |
				(procName[2] << 16) |
				(procName[3] <<  0);

			if(procNameCheck != 'ESTV')
			{
				_ERROR("### someone kludged the default process name to (%s), don't ask me for support with your install ###", procName.c_str());
			}
		}
	}

	const std::string & runtimeDir = GetRuntimeDirectory();
	std::string procPath = runtimeDir + "\\" + procName;

	if(g_options.m_altEXE.size())
	{
		procPath = g_options.m_altEXE;
		_MESSAGE("launching alternate exe (%s)", procPath.c_str());
	}
	
	_MESSAGE("procPath = %s", procPath.c_str());

	// check if the exe exists
	{
		IFileStream	fileCheck;
		if(!fileCheck.Open(procPath.c_str()))
		{
			if(usedCustomRuntimeName)
			{
				// hurr durr
				PrintLoaderError("Couldn't find %s. You have customized the runtime name via SKSE's .ini file, and that file does not exist. This can usually be fixed by removing the RuntimeName line from the .ini file.)", procName.c_str());
			}
			else
			{
				PrintLoaderError("Couldn't find %s.", procName.c_str());
			}

			return 1;
		}
	}

	_MESSAGE("launching: %s (%s)", procName.c_str(), procPath.c_str());

	if(g_options.m_altDLL.size())
	{
		baseDllName = g_options.m_altDLL.c_str();
		_MESSAGE("launching alternate dll (%s)", baseDllName);

		dllHasFullPath = true;
	}

	std::string		dllSuffix;
	ProcHookInfo	procHookInfo;

	// check exe version
	if(!IdentifyEXE(procPath.c_str(), g_options.m_launchCS, &dllSuffix, &procHookInfo))
	{
		_ERROR("unknown exe");

		if(usedCustomRuntimeName)
		{
			// hurr durr
			PrintLoaderError("You have customized the runtime name via SKSE's .ini file. Version errors can usually be fixed by removing the RuntimeName line from the .ini file.");
		}

		return 1;
	}

	_MESSAGE("hook call addr = %08X", procHookInfo.hookCallAddr);
	_MESSAGE("load lib addr = %08X", procHookInfo.loadLibAddr);

	if(g_options.m_crcOnly)
		return 0;

	// build dll path
	std::string	dllPath;
	if(dllHasFullPath)
	{
		dllPath = baseDllName;
	}
	else
	{
		dllPath = runtimeDir + "\\" + baseDllName + "_" + dllSuffix + ".dll";
	}

	_MESSAGE("dll = %s", dllPath.c_str());

	// check to make sure the dll exists
	{
		IFileStream	tempFile;

		if(!tempFile.Open(dllPath.c_str()))
		{
			PrintLoaderError("Couldn't find SKSE DLL (%s). Please make sure you have installed SKSE correctly and are running it from your Skyrim folder.", dllPath.c_str());
			return 1;
		}
	}

	// steam setup
	if(procHookInfo.procType == kProcType_Steam)
	{
		if(g_options.m_launchSteam)
		{
			// if steam isn't running, launch it
			if(!SteamCheckPassive())
			{
				_MESSAGE("steam not running, launching it");

				if(!SteamLaunch())
				{
					_WARNING("failed to launch steam");
				}
			}
		}

		// same for standard and nogore
		const char * kAppID = (g_options.m_launchCS == false ? "72850" : "202480");

		// set this no matter what to work around launch issues
		SetEnvironmentVariable("SteamGameId", kAppID);

		if(g_options.m_skipLauncher)
		{
			SetEnvironmentVariable("SteamAppID", kAppID);
		}
	}

	// launch the app (suspended)
	STARTUPINFO			startupInfo = { 0 };
	PROCESS_INFORMATION	procInfo = { 0 };

	startupInfo.cb = sizeof(startupInfo);

	if(!CreateProcess(
		procPath.c_str(),
		NULL,	// no args
		NULL,	// default process security
		NULL,	// default thread security
		FALSE,	// don't inherit handles
		CREATE_SUSPENDED,
		NULL,	// no new environment
		NULL,	// no new cwd
		&startupInfo, &procInfo))
	{
		if(GetLastError() == 740)
		{
			PrintLoaderError("Launching %s failed (%d). Please try running skse_loader as an administrator.", procPath.c_str(), GetLastError());
		}
		else
		{
			PrintLoaderError("Launching %s failed (%d).", procPath.c_str(), GetLastError());
		}

		return 1;
	}

	_MESSAGE("main thread id = %d", procInfo.dwThreadId);

	// set affinity if requested
	if(g_options.m_affinity)
	{
		_MESSAGE("setting affinity mask to %016I64X", g_options.m_affinity);

		if(!SetProcessAffinityMask(procInfo.hProcess, g_options.m_affinity))
		{
			_WARNING("couldn't set affinity mask (%08X)", GetLastError());
		}
	}

	bool	injectionSucceeded = false;

	// inject the dll
	switch(procHookInfo.procType)
	{
		case kProcType_Steam:
		{
			std::string	steamHookDllPath = runtimeDir + "\\skse_steam_loader.dll";

			injectionSucceeded = InjectDLLThread(&procInfo, steamHookDllPath.c_str(), true);
		}
		break;

		case kProcType_Normal:
			if(InjectDLL(&procInfo, dllPath.c_str(), &procHookInfo))
			{
				injectionSucceeded = true;
			}
			break;

		default:
			HALT("impossible");
	}

	// start the process if successful
	if(!injectionSucceeded)
	{
		PrintLoaderError("Couldn't inject DLL.");

		_ERROR("terminating process");

		TerminateProcess(procInfo.hProcess, 0);
	}
	else
	{
		_MESSAGE("launching");

		ResumeThread(procInfo.hThread);

		if(g_options.m_moduleInfo)
		{
			Sleep(1000 * 3);	// wait 3 seconds

			PrintModuleInfo(procInfo.dwProcessId);
		}

		if(g_options.m_waitForClose)
			WaitForSingleObject(procInfo.hProcess, INFINITE);
	}

	// clean up
	CloseHandle(procInfo.hProcess);
	CloseHandle(procInfo.hThread);

	return 0;
}

static bool InjectDLL(PROCESS_INFORMATION * info, const char * dllPath, ProcHookInfo * hookInfo)
{
	bool	result = false;

	// wrap DLL injection in SEH, if it crashes print a message
	__try {
		result = DoInjectDLL(info, dllPath, hookInfo);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		PrintLoaderError("DLL injection failed. In most cases, this is caused by an overly paranoid software firewall or antivirus package. Disabling either of these may solve the problem.");
		result = false;
	}

	return result;
}

static void PrintModuleInfo(UInt32 procID)
{
	HANDLE	snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, procID);
	if(snap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32	module;

		module.dwSize = sizeof(module);

		if(Module32First(snap, &module))
		{
			do 
			{
				_MESSAGE("%08Xx%08X %08X %s %s", module.modBaseAddr, module.modBaseSize, module.hModule, module.szModule, module.szExePath);
			}
			while(Module32Next(snap, &module));
		}
		else
		{
			_ERROR("PrintModuleInfo: Module32First failed (%d)", GetLastError());
		}

		CloseHandle(snap);
	}
	else
	{
		_ERROR("PrintModuleInfo: CreateToolhelp32Snapshot failed (%d)", GetLastError());
	}
}

static bool InjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync)
{
	bool	result = false;

	// wrap DLL injection in SEH, if it crashes print a message
	__try {
		result = DoInjectDLLThread(info, dllPath, sync);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		PrintLoaderError("DLL injection failed. In most cases, this is caused by an overly paranoid software firewall or antivirus package. Disabling either of these may solve the problem.");
		result = false;
	}

	return result;
}

static bool DoInjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync)
{
	bool	result = false;

	// make sure the dll exists
	IFileStream	fileCheck;
	if(!fileCheck.Open(dllPath))
	{
		PrintLoaderError("Couldn't find %s.", dllPath);
		return false;
	}

	fileCheck.Close();

	HANDLE	process = OpenProcess(
		PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, info->dwProcessId);
	if(process)
	{
		UInt32	hookBase = (UInt32)VirtualAllocEx(process, NULL, 8192, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if(hookBase)
		{
			// safe because kernel32 is loaded at the same address in all processes
			// (can change across restarts)
			UInt32	loadLibraryAAddr = (UInt32)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

			_MESSAGE("hookBase = %08X", hookBase);
			_MESSAGE("loadLibraryAAddr = %08X", loadLibraryAAddr);

			UInt32	bytesWritten;
			WriteProcessMemory(process, (LPVOID)(hookBase + 5), dllPath, strlen(dllPath) + 1, &bytesWritten);

			UInt8	hookCode[5];

			hookCode[0] = 0xE9;
			*((UInt32 *)&hookCode[1]) = loadLibraryAAddr - (hookBase + 5);

			WriteProcessMemory(process, (LPVOID)(hookBase), hookCode, sizeof(hookCode), &bytesWritten);

			HANDLE	thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)hookBase, (void *)(hookBase + 5), 0, NULL);
			if(thread)
			{
				if(sync)
				{
					switch(WaitForSingleObject(thread, g_options.m_noTimeout ? INFINITE : 1000 * 60))	// timeout = one minute
					{
						case WAIT_OBJECT_0:
							_MESSAGE("hook thread complete");
							result = true;
							break;

						case WAIT_ABANDONED:
							_ERROR("Process::InstallHook: waiting for thread = WAIT_ABANDONED");
							break;

						case WAIT_TIMEOUT:
							_ERROR("Process::InstallHook: waiting for thread = WAIT_TIMEOUT");
							break;
					}
				}
				else
					result = true;

				CloseHandle(thread);
			}
			else
				_ERROR("CreateRemoteThread failed (%d)", GetLastError());

			VirtualFreeEx(process, (LPVOID)hookBase, 0, MEM_RELEASE);
		}
		else
			_ERROR("Process::InstallHook: couldn't allocate memory in target process");

		CloseHandle(process);
	}
	else
		_ERROR("Process::InstallHook: couldn't get process handle");

	return result;
}
