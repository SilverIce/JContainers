#pragma once

namespace collections {

    enum constants {
        kJStorageChunk = 'JSTR',

        kJJSerializationVersionPreAQueueFix = 2,
        kJSerializationNoHeaderVersion = 3,
        kJSerializationCurrentVersion = 4,

        kJAPIVersion = 3,

        kJVersionMajor = kJAPIVersion,
        kJVersionMinor = 1,
        kJVersionPatch = 0,
    };

#   define JC_USER_FILES    "/My Games/Skyrim/JCUser/"

}
