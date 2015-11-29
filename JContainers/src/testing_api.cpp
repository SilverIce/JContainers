#include "jcontainers_constants.h"
#include "reflection/reflection.h"
#include "gtest.h"

// C API for python scripts as a part of bundling and testing functionality
extern "C" {

    __declspec(dllexport) const char * JC_versionString() {
        return JC_VERSION_STR;
    }

    __declspec(dllexport) void JC_produceCode(const char *directoryPath) {

        using namespace reflection;

        const std::string path = directoryPath ? directoryPath : "";
        foreach_metaInfo_do([&](const class_info& info) {
            code_producer::produceClassToFile(info, path);
        });
    }

    __declspec(dllexport) bool JC_runTests(int argc, char** argv) {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS() == 0;
    }
}
