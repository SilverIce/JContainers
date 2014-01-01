// PIZDA.cpp : Defines the entry point for the console application.
//

#include "windows.h"
#include <tchar.h>

typedef void (*ShityTests)();

int _tmain(int argc, _TCHAR* argv[])
{
    HMODULE handle = (HMODULE)LoadLibrary(L"JContainers.dll");
    if(handle) {
       ShityTests test = (ShityTests)GetProcAddress(handle, "launchShityTest");
       test();
    }


	return 0;
}

