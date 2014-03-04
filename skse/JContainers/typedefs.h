#pragma once

enum {
    kJStorageChunk = 'JSTR',

    kJSerializationDataVersion = 1,
    kJAPIVersion = 1,
};

#define STR(...) #__VA_ARGS__

namespace Movement
{
    typedef signed char     int8;
    typedef unsigned char   uint8;
    typedef short           int16;
    typedef unsigned short  uint16;
    typedef int             int32;
    typedef unsigned int    uint32;
    typedef __int64         int64;
    typedef unsigned __int64 uint64;

}
