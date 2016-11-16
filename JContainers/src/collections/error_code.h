#pragma once

namespace collections {

    enum JErrorCode {
        JError_NoError = 0,
        JError_ArrayOutOfBoundAccess,
        //JError_UnableOpenFile,
        //JError_UnableParseJSON,
        
        JErrorCount,
    };

    static inline const char * JErrorCodeToString(JErrorCode code) {
        struct code2Str {
            JErrorCode code;
            const char *string;
        };

        const code2Str codes [] = {
            JError_NoError, STR(JError_NoError),
            JError_ArrayOutOfBoundAccess, STR(JError_ArrayOutOfBoundAccess),
            //JError_UnableOpenFile, STR(JError_UnableOpenFile),
            //JError_UnableParseJSON, STR(JError_UnableParseJSON),
        };

        for (auto& inf : codes) {
            if (inf.code == code) {
                return inf.string;
            }
        }

        return "Invalid error code";
    }
}
