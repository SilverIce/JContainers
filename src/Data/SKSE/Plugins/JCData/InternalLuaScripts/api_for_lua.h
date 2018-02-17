#pragma once

// C interface for LuaJIT FFI

const char* JC_hello();

typedef const char * cstring;
typedef int32_t index;
typedef struct _CForm { uint32_t ___id;} CForm;

typedef void * handle;

typedef struct _CString {
    const char* str;
    size_t length;
} CString;

void CString_free(CString *str);

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


void JCToLuaValue_free(JCToLuaValue* v);

void* JC_get_c_function(const char *func, const char * classname);

handle JValue_retain(handle obj);
handle JValue_release(handle obj);
handle JValue_count(handle obj);
uint32_t JValue_typeId(handle obj);
JCToLuaValue JValue_solvePath(handle context, handle obj, cstring path);

JCToLuaValue JArray_getValue(handle obj, index key);
void JArray_setValue(handle obj, index key, const JCValue* val);
void JArray_insert(handle obj, JCValue* val, index key);

//////////////////////////////////////////////////////////////////////////

void JMap_setValue(handle, cstring key, const JCValue* val);
JCToLuaValue JMap_getValue(handle, cstring key);
CString JMap_nextKey(handle, cstring lastKey);

//////////////////////////////////////////////////////////////////////////

CForm JFormMap_nextKey(handle obj, CForm lastKey);
void JFormMap_setValue(handle obj, CForm key, const JCValue* val);
JCToLuaValue JFormMap_getValue(handle obj, CForm key);
void JFormMap_removeKey(handle obj, CForm key);

handle JDB_instance(handle jc_context);
