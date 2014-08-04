#include <boost/filesystem/path.hpp>
#include <windef.h>

static void init_boost() {
    boost::filesystem::path p("dummy");
};

BOOL APIENTRY DllMain(HMODULE /* hModule */,
    DWORD  ul_reason_for_call,
    LPVOID /* lpReserved */)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        init_boost();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
