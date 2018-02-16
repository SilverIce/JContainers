#pragma once

#include "skse64/PapyrusArgs.h"
#include "skse64/PapyrusStringUtil.h"

template <> inline UInt64 GetTypeID <skse::string_ref>(VMClassRegistry * registry)                        { return VMValue::kType_String; }
template <> inline UInt64 GetTypeID <VMArray<skse::string_ref>>(VMClassRegistry * registry)       { return VMValue::kType_StringArray; }
template <> inline UInt64 GetTypeID <VMResultArray<skse::string_ref>>(VMClassRegistry * registry) { return VMValue::kType_StringArray; }


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
