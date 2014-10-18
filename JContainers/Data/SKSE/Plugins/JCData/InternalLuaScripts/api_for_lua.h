#pragma once

// C interface for LuaJIT FFI

__declspec(dllexport) const char* JC_hello();

typedef const char * cstring;
typedef int32_t index;
typedef struct _CForm { uint32_t ___id;} CForm;

typedef void * handle;
//typedef struct _JCObject { handle id; } JCObject;

typedef struct _CString {
    const char* str;
    size_t length;
} CString;

__declspec(dllexport) void CString_free(CString *str);

typedef struct _JCToLuaValue {
    uint32_t type;
    union {
        cstring     string;
        CForm       form;
        int32_t     integer;
        double      real;
        handle      object;
    };
    uint32_t stringLength;
} JCToLuaValue;

typedef struct _JCValue {
    uint32_t type;
    union {
        CForm       form;
        cstring     string;
        int32_t     integer;
        double      real;
        handle      object;
    };
    uint32_t stringLength;
} JCValue;



__declspec(dllexport) void JCToLuaValue_free(JCToLuaValue* v);

void* JC_get_c_function(const char *func, const char * classname);

__declspec(dllexport) handle JValue_retain(handle obj);
__declspec(dllexport) handle JValue_release(handle obj);
__declspec(dllexport) handle JValue_count(handle obj);
__declspec(dllexport) uint32_t JValue_typeId(handle obj);

__declspec(dllexport) JCToLuaValue JArray_getValue(handle obj, index key);
__declspec(dllexport) void JArray_setValue(handle obj, index key, const JCValue* val);

//////////////////////////////////////////////////////////////////////////

__declspec(dllexport) void JMap_setValue(handle, cstring key, const JCValue* val);
__declspec(dllexport) JCToLuaValue JMap_getValue(handle, cstring key);
__declspec(dllexport) CString JMap_nextKey(handle, cstring lastKey);

//////////////////////////////////////////////////////////////////////////

__declspec(dllexport) CForm JFormMap_nextKey(handle obj, CForm lastKey);
__declspec(dllexport) void JFormMap_setValue(handle obj, CForm key, const JCValue* val);
__declspec(dllexport) JCToLuaValue JFormMap_getValue(handle obj, CForm key);


