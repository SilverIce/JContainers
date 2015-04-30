#pragma once

#include "skse/PapyrusArgs.h"
#include "skse_string.h"

template <> inline UInt32 GetTypeID <skse::string_ref>(VMClassRegistry * registry)                        { return VMValue::kType_String; }
template <> inline UInt32 GetTypeID <VMArray<skse::string_ref>>(VMClassRegistry * registry)       { return VMValue::kType_StringArray; }
template <> inline UInt32 GetTypeID <VMResultArray<skse::string_ref>>(VMClassRegistry * registry) { return VMValue::kType_StringArray; }


template <> inline void PackValue <skse::string_ref>(VMValue * dst, skse::string_ref * src, VMClassRegistry * registry)
{
    dst->SetString(src->c_str());
}

template <> inline void UnpackValue <skse::string_ref>(skse::string_ref * dst, VMValue * src)
{
    const char      * data = NULL;

    if (src->type == VMValue::kType_String)
        data = src->data.str;

    *dst = data;
}

template <> inline  void UnpackValue <VMArray<skse::string_ref>>(VMArray<skse::string_ref> * dst, VMValue * src)
{
    UnpackArray(dst, src, VMValue::kType_StringArray);
}
