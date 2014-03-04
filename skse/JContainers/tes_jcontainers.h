namespace collections {
    class tes_jcontainers : public tes_binding::class_meta_mixin<tes_jcontainers> {
    public:

        REGISTER_TES_NAME("JContainers");

        static void additionalSetup() {
            metaInfo().comment = "Various utility methods";
        }

        static bool isInstalled() {
            return true;
        }
        REGISTERF2(isInstalled, NULL, "returns true if JContainers plugin is installed");

        static UInt32 APIVersion() {
            return kJAPIVersion;
        }
        REGISTERF2(APIVersion, NULL, "returns API version. Incremented by 1 each time old API is not backward compartible with new one");

        static bool fileExistsAtPath(const char *filename) {
            struct _stat buf;
            int result = _stat(filename, &buf);
            return result == 0;
        }
        REGISTERF2(fileExistsAtPath, "path", "returns true if file at path exists");

        static SInt32 lastError() {
            return shared_state::instance().lastError();
        }
        REGISTERF2(lastError, NULL, "");

        static const char* lastErrorString() {
            return JErrorCodeToString(shared_state::instance().lastError());
        }
        REGISTERF2(lastErrorString, NULL, "");
    };

}
