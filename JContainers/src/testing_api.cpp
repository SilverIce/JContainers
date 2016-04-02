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

        code_producer::produceAmalgamatedCodeToFile(reflection::class_registry(), path);
    }

    __declspec(dllexport) bool JC_runTests(int argc, const char** argv) {
        using namespace std;

        // @argv points to Python internal memory, it's best to keep things safe and copy it
        const vector<string> args(argv, argv + argc);
        vector<char*> char_ptr_args;
        transform(args.begin(), args.end(), back_inserter(char_ptr_args),
            [](const string& s) { return const_cast<char*>(s.c_str()); });

        ::testing::InitGoogleTest(&argc, &char_ptr_args.front());
        return RUN_ALL_TESTS() == 0;
    }
}
