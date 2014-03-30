#include <string>
#include <direct.h>
#include <shlobj.h>
#include "skse/skse_version.h"
#include "skse/SafeWrite.h"
#include "skse/Utilities.h"
#include "common/IFileStream.h"
#include "loader_common/IdentifyEXE.h"
#include <tlhelp32.h>

// hack for VS2005, intrin.h can't be included with winnt.h
#if _MSC_VER <= 1400
#define _interlockedbittestandreset _interlockedbittestandreset_NAME_CHANGED_TO_AVOID_MSVC2005_ERROR
#define _interlockedbittestandset _interlockedbittestandset_NAME_CHANGED_TO_AVOID_MSVC2005_ERROR
#endif

#include <intrin.h>

IDebugLog	gLog;
HANDLE		g_dllHandle;

static void OnAttach(void);

std::string GetAppPath(void)
{
	char	appPath[4096];

	ASSERT(GetModuleFileName(GetModuleHandle(NULL), appPath, sizeof(appPath)));

	return appPath;
}

std::string GetAppDir(void)
{
	std::string	appPath = GetAppPath();

	std::string::size_type slashOffset = appPath.rfind('\\');
	if(slashOffset == std::string::npos)
		return appPath;

	return appPath.substr(0, slashOffset);
}

bool RunningEditor(void)
{
	// ### this is not a good method of detection
	char pathBuffer[MAX_PATH] = {0};

	strcpy_s(pathBuffer, sizeof(pathBuffer), GetAppPath().c_str());
	_strlwr_s(pathBuffer, sizeof(pathBuffer));

	return strstr(pathBuffer, "creationkit.exe") != NULL;
}

BOOL WINAPI DllMain(HANDLE procHandle, DWORD reason, LPVOID reserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		g_dllHandle = procHandle;

		OnAttach();
	}

	return TRUE;
}

typedef int (__stdcall * _HookedWinMain)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
_HookedWinMain g_hookedWinMain = NULL;
std::string g_dllPath;

int __stdcall OnHook(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	_MESSAGE("OnHook: thread = %d", GetCurrentThreadId());

	// load main dll
	if(!LoadLibrary(g_dllPath.c_str()))
	{
		_ERROR("couldn't load dll (%08X)", GetLastError());
	}

	// call original winmain
	_MESSAGE("calling winmain");

	int result = g_hookedWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	_MESSAGE("returned from winmain (%d)", result);

	return result;
}

void DumpThreads(void)
{
	HANDLE	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
	if(snapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32	info;

		info.dwSize = sizeof(info);

		if(Thread32First(snapshot, &info))
		{
			do 
			{
				if(info.th32OwnerProcessID == GetCurrentProcessId())
				{
					UInt32	eip = 0xFFFFFFFF;

					HANDLE	thread = OpenThread(THREAD_GET_CONTEXT, FALSE, info.th32ThreadID);
					if(thread)
					{
						CONTEXT	ctx;

						ctx.ContextFlags = CONTEXT_CONTROL;

						GetThreadContext(thread, &ctx);

						eip = ctx.Eip;

						CloseHandle(thread);
					}

					_MESSAGE("thread %d pri %d eip %08X%s",
						info.th32ThreadID,
						info.tpBasePri,
						eip,
						(info.th32ThreadID == GetCurrentThreadId()) ? " current thread" : "");
				}

				info.dwSize = sizeof(info);
			}
			while(Thread32Next(snapshot, &info));
		}

		CloseHandle(snapshot);
	}
}

bool hookInstalled = false;

void InstallHook(void * retaddr, UInt32 hookSrc)
{
	if(hookInstalled)
		return;
	else
		hookInstalled = true;

	_MESSAGE("InstallHook: thread = %d retaddr = %08X hookSrc = %d", GetCurrentThreadId(), retaddr, hookSrc);

//	DumpThreads();

	std::string appPath = GetAppPath();
	_MESSAGE("appPath = %s", appPath.c_str());

	std::string		dllSuffix;
	ProcHookInfo	procHookInfo;
	bool			isEditor = RunningEditor();

	if(!IdentifyEXE(appPath.c_str(), isEditor, &dllSuffix, &procHookInfo))
	{
		_ERROR("unknown exe");
		return;
	}

	// build full path to our dll
	const char	* dllPrefix = (isEditor == false) ? "\\skse_" : "\\skse_editor_";

	g_dllPath = GetAppDir() + dllPrefix + dllSuffix + ".dll";
	_MESSAGE("dll = %s", g_dllPath.c_str());

	// hook winmain call
	UInt32	hookBaseAddr = procHookInfo.hookCallAddr;
	UInt32	hookBaseCallAddr = *((UInt32 *)(hookBaseAddr + 1));

	hookBaseCallAddr += 5 + hookBaseAddr;	// adjust for relcall

	_MESSAGE("old winmain = %08X", hookBaseCallAddr);

	g_hookedWinMain = (_HookedWinMain)hookBaseCallAddr;

	UInt32	newHookDst = ((UInt32)OnHook) - hookBaseAddr - 5;

	SafeWrite32(hookBaseAddr + 1, newHookDst);

	FlushInstructionCache(GetCurrentProcess(), NULL, 0);
}

typedef void (__stdcall * _GetSystemTimeAsFileTime)(LPFILETIME * fileTime);

_GetSystemTimeAsFileTime GetSystemTimeAsFileTime_Original = NULL;
_GetSystemTimeAsFileTime * _GetSystemTimeAsFileTime_IAT = NULL;

typedef void (__stdcall * _GetStartupInfoA)(STARTUPINFOA * startupInfo);

_GetStartupInfoA GetStartupInfoA_Original = NULL;
_GetStartupInfoA * _GetStartupInfoA_IAT = NULL;

void __stdcall GetSystemTimeAsFileTime_Hook(LPFILETIME * fileTime)
{
	InstallHook(_ReturnAddress(), 0);

	GetSystemTimeAsFileTime_Original(fileTime);
}

void __stdcall GetStartupInfoA_Hook(STARTUPINFOA * startupInfo)
{
	InstallHook(_ReturnAddress(), 1);

	GetStartupInfoA_Original(startupInfo);
}

static void OnAttach(void)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim\\SKSE\\skse_steam_loader.log");
	gLog.SetPrintLevel(IDebugLog::kLevel_Error);
	gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

	FILETIME	now;
	GetSystemTimeAsFileTime(&now);

	_MESSAGE("skse loader %08X (steam) %08X%08X %s", PACKED_SKSE_VERSION, now.dwHighDateTime, now.dwLowDateTime, GetOSInfoStr().c_str());
	_MESSAGE("base addr = %08X", g_dllHandle);

	UInt32	oldProtect;

	_GetSystemTimeAsFileTime_IAT = (_GetSystemTimeAsFileTime *)GetIATAddr((UInt8 *)GetModuleHandle(NULL), "kernel32.dll", "GetSystemTimeAsFileTime");
	if(_GetSystemTimeAsFileTime_IAT)
	{
		_MESSAGE("GetSystemTimeAsFileTime IAT = %08X", _GetSystemTimeAsFileTime_IAT);

		VirtualProtect((void *)_GetSystemTimeAsFileTime_IAT, 4, PAGE_EXECUTE_READWRITE, &oldProtect);

		_MESSAGE("original GetSystemTimeAsFileTime = %08X", *_GetSystemTimeAsFileTime_IAT);
		GetSystemTimeAsFileTime_Original = *_GetSystemTimeAsFileTime_IAT;
		*_GetSystemTimeAsFileTime_IAT = GetSystemTimeAsFileTime_Hook;
		_MESSAGE("patched GetSystemTimeAsFileTime = %08X", *_GetSystemTimeAsFileTime_IAT);

		UInt32 junk;
		VirtualProtect((void *)_GetSystemTimeAsFileTime_IAT, 4, oldProtect, &junk);
	}
	else
	{
		_ERROR("couldn't read IAT");
	}

	// win8 automatically initializes the stack cookie, so the previous hook doesn't get hit
	_GetStartupInfoA_IAT = (_GetStartupInfoA *)GetIATAddr((UInt8 *)GetModuleHandle(NULL), "kernel32.dll", "GetStartupInfoA");
	if(_GetStartupInfoA_IAT)
	{
		_MESSAGE("GetStartupInfoA IAT = %08X", _GetStartupInfoA_IAT);

		VirtualProtect((void *)_GetStartupInfoA_IAT, 4, PAGE_EXECUTE_READWRITE, &oldProtect);

		_MESSAGE("original GetStartupInfoA = %08X", *_GetStartupInfoA_IAT);
		GetStartupInfoA_Original = *_GetStartupInfoA_IAT;
		*_GetStartupInfoA_IAT = GetStartupInfoA_Hook;
		_MESSAGE("patched GetStartupInfoA = %08X", *_GetStartupInfoA_IAT);

		UInt32 junk;
		VirtualProtect((void *)_GetStartupInfoA_IAT, 4, oldProtect, &junk);
	}
}
