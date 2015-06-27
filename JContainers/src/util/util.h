#pragma once

#include <chrono>
#include <assert.h>
#include "typedefs.h"

namespace boost { namespace filesystem {

    class path;
}}

extern void _DMESSAGE(const char * fmt, ...);

namespace util {

    boost::filesystem::path dll_path();
    boost::filesystem::path relative_to_dll_path(const char *relative_path);

    template<class T>
    void do_with_timing(const char *operation_name, T& func) {
        assert(operation_name);
        _DMESSAGE("%s started", operation_name);

        namespace chr = std::chrono;
        auto started = chr::system_clock::now();

        try {
            func();
        }
        catch (const std::exception& ex) {
            _ERROR("'%s' throws '%s' of type '%s'", operation_name, ex.what(), typeid(ex).name());
            jc_assert(false);
        }
        catch (...) {
            _ERROR("'%s' throws unk. exception", operation_name);
            jc_assert(false);
        }

        auto ended = chr::system_clock::now();
        float diff = chr::duration_cast<chr::milliseconds>(ended - started).count() / 1000.f;
        _DMESSAGE("%s finished in %f sec", operation_name, diff);
    }
}
