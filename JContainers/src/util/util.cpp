#include <boost/filesystem/path.hpp>
#include <windef.h>

#include "gtest.h"
#include "util/string_normalize.h"

namespace util {

#define countof(array) sizeof(array)/(sizeof(array[0]))

    boost::filesystem::path dll_path() {
        HMODULE hm = nullptr;
        auto result = GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&dll_path,
            &hm);
        assert(result && "GetModuleHandleExA failed");
        wchar_t path[MAX_PATH] = { '\0' };
        GetModuleFileNameW(hm, path, countof(path));
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

namespace util {
    namespace {

        TEST(string_normalizer, _) {
            // ensure that string_normalizer::normalize doesn't
            // create gaps - zero bytes

            for (uint32_t i = 1; i <= 0xff; ++i) {
                auto norm = string_normalizer::normalize(i);
                if (norm <= 0xff) {
                    EXPECT_TRUE((uint8_t)norm != 0);
                }
                else {
                    EXPECT_TRUE((uint8_t)norm != 0);
                    EXPECT_TRUE((uint8_t)(norm >> 8) != 0);
                }
            }
        }

    }
}
