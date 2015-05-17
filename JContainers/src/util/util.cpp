#include <boost/filesystem/path.hpp>
#include <windef.h>

namespace util {

    boost::filesystem::path dll_path() {
        HMODULE hm = nullptr;
        bool result = GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&dll_path,
            &hm);
        assert(result && "GetModuleHandleExA failed");
        wchar_t path[MAX_PATH] = { '\0' };
        GetModuleFileNameW(hm, path, sizeof path);
        return boost::filesystem::path(path);
    }

    boost::filesystem::path relative_to_dll_path(const char *relative_path) {
        assert(relative_path);
        auto imagePath = dll_path();
        return (imagePath.remove_filename() /= relative_path);
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
