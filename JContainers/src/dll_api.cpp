#include "jcontainers_constants.h"
#include "reflection.h"
#include "gtest.h"

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

    __declspec(dllexport) void JC_runTests() {
        testing::runTests(meta<testing::TestInfo>::getListConst());
    }
}
