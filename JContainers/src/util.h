#pragma once

namespace boost { namespace filesystem {

    class path;
}}

namespace util {

    boost::filesystem::path dll_path();
    boost::filesystem::path relative_to_dll_path(const char *relative_path);
}
