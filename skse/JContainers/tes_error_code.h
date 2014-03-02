#pragma once

namespace collections {

    enum JErrorCode {
        JError_None = 0,
        JError_OutOfBoundAccess,
        JError_UnableOpenFile,
        JError_UnableParseJSON,
    };

    static inline const char * JErrorCodeToString(JErrorCode code) {
        struct code2Str {
            JErrorCode code;
            const char *string;
        };

        const code2Str codes [] = {
            JError_None, STR(JError_None),
            JError_OutOfBoundAccess, STR(JError_OutOfBoundAccess),
            JError_UnableOpenFile, STR(JError_UnableOpenFile),
            JError_UnableParseJSON, STR(JError_UnableParseJSON),
        };

        for (auto& inf : codes) {
            if (inf.code == code) {
                return inf.string;
            }
        }

        return "Invalid error code";
    }
}
