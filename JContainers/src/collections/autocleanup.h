#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace collections {

    class tes_context;
    class object_base;

    // functionality boosted from Papyrus API level up to collection level so that I can
    // invoke @u_autocleanup functionality from tes_context level
    namespace autocleanup {

        void setPathForAutoremoval(tes_context& context, const char* pluginNameUnsafe, const char* path);
        void unsetPathForAutoremoval(tes_context& context, const char* pluginNameUnsafe, const char* path);

        void u_autocleanup(tes_context& context, std::function<bool(const std::string&)>&& isPluginLoaded);
        void u_autocleanup(tes_context& context);
    }
}
