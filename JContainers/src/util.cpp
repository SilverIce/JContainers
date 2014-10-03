#include <boost/filesystem/path.hpp>
#include <windef.h>

namespace util {

    boost::filesystem::path relative_to_dll_path(const char *relative_path) {
        assert(relative_path);

        HMODULE hm = nullptr;
        assert(GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&relative_to_dll_path,
            &hm));

        char path[MAX_PATH] = { '\0' };
        GetModuleFileNameA(hm, path, sizeof path);
        return (boost::filesystem::path(path).remove_filename() /= relative_path);
    }
}

//////////////////////////////////////////////////////////////////////////
// boost 'fix'
//////////////////////////////////////////////////////////////////////////

static void init_boost() {
    boost::filesystem::path p("dummy");
}

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
