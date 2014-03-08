namespace collections {

    class tes_jcontainers : public tes_binding::class_meta_mixin_t<tes_jcontainers> {
    public:

        REGISTER_TES_NAME("JContainers");

        void additionalSetup() {
            metaInfo.comment = "Various utility methods";
        }

        static bool isInstalled() {
            return true;
        }
        REGISTERF2(isInstalled, NULL, "returns true if JContainers plugin is installed");

        static UInt32 APIVersion() {
            return kJAPIVersion;
        }
        REGISTERF2(APIVersion, NULL, []() {
            std::stringstream comm;
            comm << "returns API version. Incremented by 1 each time old API is not backward compartible with new one.\n";
            comm << "current API version is " << APIVersion();
            return comm.str();
        });

        static bool fileExistsAtPath(const char *filename) {
            struct _stat buf;
            int result = _stat(filename, &buf);
            return result == 0;
        }
        REGISTERF2(fileExistsAtPath, "path", "returns true if file at path exists");

        static SInt32 lastError() {
            return tes_context::instance().lastError();
        }
        REGISTERF2(lastError, NULL, []() {
            std::stringstream comm;
            comm << "returns last occured error (error code):";
            for (int i = 0; i < JErrorCount; ++i) {
               comm << std::endl << i << " - " << JErrorCodeToString((JErrorCode)i);
            }
            return comm.str();
        });

        static const char* lastErrorString() {
            return JErrorCodeToString(tes_context::instance().lastError());
        }
        REGISTERF2(lastErrorString, NULL, "returns string that describes last error");
    };

    TES_META_INFO(tes_jcontainers);
}
