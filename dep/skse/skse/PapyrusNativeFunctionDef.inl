#define CLASS_NAME __MACRO_JOIN__(NativeFunction, NUM_PARAMS)

#define VOID_SPEC 0
#include "PapyrusNativeFunctionDef_Base.inl"
#define VOID_SPEC 1
#include "PapyrusNativeFunctionDef_Base.inl"

#undef CLASS_NAME
#define CLASS_NAME __MACRO_JOIN__(NativeFunctionWithState, NUM_PARAMS)
#define ACCEPTS_STATE

#define VOID_SPEC 0
#include "PapyrusNativeFunctionDef_Base.inl"
#define VOID_SPEC 1
#include "PapyrusNativeFunctionDef_Base.inl"

#undef ACCEPTS_STATE

#undef CLASS_NAME
#undef NUM_PARAMS
