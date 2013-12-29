#include "skse/PapyrusVM.h"
#include "skse/PapyrusNativeFunctions.h"

#include "cJSON/cJSON.h"
#include "registry.h"

#include <functional>
#include <tuple>

namespace JValue {
    // static unsigned long create(StaticFunctionTag *) {

    typedef SInt32 int32;
    typedef UInt32 uint32;

#define check(exprr) \
    if (!(exprr)) { \
        _DMESSAGE("check " #exprr " failed"); \
    }

    typedef Registry<cJSON> JRegistry;

    void print(const char *h) {
        printf(h);
    }
    /*
    template<typename Func, Func f, typename... Args >
    static void tes_print(Args&& ... args) {


         f(std::forward<Args>(args) ...);
    }
    */

#pragma region construction

    static uint32 parse(StaticFunctionTag*, BSFixedString val) {
        _DMESSAGE(__FUNCTION__);
        if (!val.data)
            return 0;

        FILE *file = fopen(val.data, "r");
        if (!file)
            return 0;

        char buffer[1024];
        std::vector<char> bytes;
        size_t readen = 0;
        while (!ferror(file) && !feof(file)) {

            readen = fread(buffer, 1, sizeof(buffer), file);
            if (readen > 0)
            {
                bytes.insert(bytes.end(), buffer, buffer + readen);
            }
            else
            {
                break;
            }
        }
        bytes.push_back(0);

        return JRegistry::registerObject( cJSON_Parse(&bytes[0]) );
    }

    static void writeTo(StaticFunctionTag*, uint32 hdl, BSFixedString val) {
        _DMESSAGE(__FUNCTION__);
        auto obj = JRegistry::getObject(hdl);
        if (!obj) return;
        
        char *data = cJSON_Print(obj);
        if (!data) return;

        FILE *file = fopen(val.data, "w");
        if (file) {
            fwrite(data, 1, strlen(data), file);
            fclose(file);
        }

        free(data);
    }

    static void JValueDelete(StaticFunctionTag*, uint32 val) {
        _DMESSAGE(__FUNCTION__);
        cJSON_Delete(JRegistry::getObject(val));
    }

    static uint32 createStr(StaticFunctionTag*, BSFixedString val) {
        return JRegistry::registerObject(cJSON_CreateString(val.data));
    }

    static uint32 createInt(StaticFunctionTag*, int32 val) {
        return JRegistry::registerObject(cJSON_CreateNumber(val));
    }

    static uint32 createFlt(StaticFunctionTag*, float val) {
        return JRegistry::registerObject(cJSON_CreateNumber(val));
    }

    static uint32 createArray(StaticFunctionTag*) {
        return JRegistry::registerObject(cJSON_CreateArray());
    }

    static uint32 createObject(StaticFunctionTag*) {
        return JRegistry::registerObject(cJSON_CreateObject());
    }
#pragma endregion

    template<class T> T fromTes(const T& value) { return value;}
    const char * fromTes(const BSFixedString& value) { return value.data;}

#pragma region map getters

    static float getFlt(StaticFunctionTag*, uint32 handle, BSFixedString key) {
        auto obj = JRegistry::getObject(handle);
        obj = obj ? cJSON_GetObjectItem(obj, key.data) : 0;
        return obj ? obj->valuedouble : 0;
    }

    static BSFixedString getStr(StaticFunctionTag*, uint32 handle, BSFixedString key) {
        auto obj = JRegistry::getObject(handle);
        float val = 0;
        obj = obj ? cJSON_GetObjectItem(obj, key.data) : 0;
        return BSFixedString( obj ? obj->valuestring : 0);
    }

    static int32 getInt(StaticFunctionTag*, uint32 handle, BSFixedString key) {
        auto obj = JRegistry::getObject(handle);
        float val = 0;
        obj = obj ? cJSON_GetObjectItem(obj, key.data) : 0;
        return obj ? obj->valueint : 0;
    }

    static int32 getObject(StaticFunctionTag*, uint32 handle, BSFixedString key) {
        auto obj = JRegistry::getObject(handle);
        return obj ? JRegistry::registerObject(cJSON_GetObjectItem(obj, key.data)) : 0;
    }
#pragma endregion


#pragma region array getters
    static uint32 objectAtIndex(StaticFunctionTag*, uint32 handle, uint32 index) {
        auto obj = JRegistry::getObject(handle);
        return obj ? JRegistry::registerObject(cJSON_GetArrayItem(obj, index)) : 0;
    }

    static float fltAtIndex(StaticFunctionTag*, uint32 handle, uint32 index) {
        auto obj = JRegistry::getObject(handle);
        obj = obj ? cJSON_GetArrayItem(obj, index) : 0;
        return obj ? obj->valuedouble : 0;
    }

    static int32 intAtIndex(StaticFunctionTag*, uint32 handle, uint32 index) {
        auto obj = JRegistry::getObject(handle);
        obj = obj ? cJSON_GetArrayItem(obj, index) : 0;
        return obj ? obj->valueint : 0;
    }

    static BSFixedString strAtIndex(StaticFunctionTag*, uint32 handle, uint32 index) {
        auto obj = JRegistry::getObject(handle);
        obj = obj ? cJSON_GetArrayItem(obj, index) : 0;
        return obj ? obj->valuestring : 0;
    }
#pragma endregion

#pragma region map setters
    void cJSON_replaceOrAddForKey(uint32 handle, const BSFixedString& key, cJSON *value) {
        auto obj = JRegistry::getObject(handle);
        if (!obj || !value)
            return;

        if (!cJSON_GetObjectItem(obj, key.data)) {
            cJSON_AddItemToObject(obj, key.data, value);
        }else {
            cJSON_ReplaceItemInObject(obj, key.data, value);
        }
    }

    template<class T>
    static void setNum(StaticFunctionTag*, uint32 handle, BSFixedString key, T val) {
        cJSON_replaceOrAddForKey(handle, key, cJSON_CreateNumber(val));
    }

    static void setStr(StaticFunctionTag*, uint32 handle, BSFixedString key, BSFixedString val) {
        cJSON_replaceOrAddForKey(handle, key, cJSON_CreateString(val.data));
    }

    static void setObjectForKey(StaticFunctionTag*, uint32 handle, BSFixedString key, uint32 val) {
        cJSON_replaceOrAddForKey(handle, key, JRegistry::getObject(val));
    }
#pragma endregion


#pragma region array setters
    void cJSON_replaceAtIndex(uint32 handle, uint32 idx, cJSON *value) {
        auto obj = JRegistry::getObject(handle);
        if (!obj || !value)
            return;

        cJSON_ReplaceItemInArray(obj, idx, value);
    }

    void cJSON_addItem(uint32 handle, cJSON *value) {
        auto obj = JRegistry::getObject(handle);
        if (!obj || !value)
            return;

        cJSON_AddItemToArray(obj, value);
    }

    static void setObjectAtIndex(StaticFunctionTag*, uint32 handle, uint32 index,  uint32 val) {
        cJSON_replaceAtIndex(handle, index, JRegistry::getObject(val));
    }

    static void setStrAtIndex(StaticFunctionTag*, uint32 handle, uint32 index,  BSFixedString val) {
        cJSON_replaceAtIndex(handle, index, cJSON_CreateString(val.data));
    }

    template<class T>
    static void setNumAtIndex(StaticFunctionTag*, uint32 handle, uint32 index,  T val) {
        cJSON_replaceAtIndex(handle, index, cJSON_CreateNumber(val));
    }
#pragma endregion

#pragma region array add& delete & detach
    template<class T>
    static void addNum(StaticFunctionTag*, uint32 handle,  T val) {
        cJSON_addItem(handle, cJSON_CreateNumber(val));
    }
    static void addStr(StaticFunctionTag*, uint32 handle,  BSFixedString val) {
        cJSON_addItem(handle, cJSON_CreateString(val.data));
    }
    static void addObj(StaticFunctionTag*, uint32 handle,  uint32 val) {
        cJSON_addItem(handle, JRegistry::getObject(val));
    }

    static void deleteObjAt(StaticFunctionTag*, uint32 handle,  uint32 idx) {
        auto obj = JRegistry::getObject(handle);
        if (!obj)
            return;

        cJSON_DeleteItemFromArray(obj, idx);
    }

    static uint32 detachObjAt(StaticFunctionTag*, uint32 handle, uint32 idx) {
        auto obj = JRegistry::getObject(handle);
        if (!obj)
            return 0;

        return JRegistry::registerObject(cJSON_DetachItemFromArray(obj, idx));
    }
#pragma endregion


#pragma region map remove & detach
    static void deleteObjAtKey(StaticFunctionTag*, uint32 handle,  BSFixedString key) {
        auto obj = JRegistry::getObject(handle);
        if (!obj)
            return;

        cJSON_DeleteItemFromObject(obj, key.data);
    }

    static uint32 detachObjAtKey(StaticFunctionTag*, uint32 handle,  BSFixedString key) {
        auto obj = JRegistry::getObject(handle);
        if (!obj)
            return 0;

        return JRegistry::registerObject(cJSON_DetachItemFromObject(obj, key.data));
    }
#pragma endregion

    static uint32 JValueArrayCount(StaticFunctionTag*, uint32 handle) {
        auto obj = JRegistry::getObject(handle);
        if (!obj)
            return 0;

        return cJSON_GetArraySize(obj);
    }

    void JValueFree(cJSON *ptr) {
        if (ptr) {
            cJSON *js = (cJSON*)ptr;
            if (js->id) {
                JRegistry::removeObject(js->id);
            }
        }
    }


    bool registerJSONFuncs(VMClassRegistry* registry)
    {
        cJSON_Hooks hooks = {operator new, operator delete, JValueFree};
        cJSON_InitHooks(&hooks);

        _DMESSAGE("register array funcs");

        FILE *file = fopen("ttttttt.txt", "w");
        fprintf(file, "helloooo");
        fclose(file);
        
        #define REGISTER2(name, func, argCount, ... /*types*/ ) \
    registry->RegisterFunction( \
        new NativeFunction ## argCount <StaticFunctionTag,  __VA_ARGS__ >(name, "JValue", func, registry)); \
            registry->SetFunctionFlags("JValue", name, VMClassRegistry::kFunctionFlag_NoWait);\

#define REGISTER(func, ret, argCount,  ... /*types*/ ) REGISTER2(#func, func, ret, argCount, __VA_ARGS__)

        REGISTER(createArray, 0, uint32);
        REGISTER(createObject, 0, uint32);
        REGISTER(createFlt, 1, uint32, float);
        REGISTER(createInt, 1, uint32, int32);
         REGISTER(createStr, 1, uint32, BSFixedString);
         REGISTER(parse, 1, uint32, BSFixedString);
         REGISTER(writeTo, 2, void, uint32, BSFixedString);

         REGISTER2("delete", JValueDelete, 1, void, uint32);

        REGISTER(deleteObjAtKey, 2, void, uint32, BSFixedString);
        REGISTER(detachObjAtKey, 2, uint32, uint32, BSFixedString);

        REGISTER2("addFlt", addNum<float>, 2, void, uint32, float);
        REGISTER2("addInt", addNum<float>, 2, void, uint32, float);
        REGISTER2("addStr", addStr, 2, void, uint32, BSFixedString);
        REGISTER(addObj, 2, void, uint32, uint32);
        REGISTER(deleteObjAt, 2, void, uint32, uint32);
        REGISTER(detachObjAt, 2, uint32, uint32, uint32);

        REGISTER2("setFltAtIndex", setNumAtIndex<float>, 3, void, uint32, uint32, float);
        REGISTER2("setIntAtIndex", setNumAtIndex<int32>, 3, void, uint32, uint32, int32);
        REGISTER(setObjectAtIndex, 3, void, uint32, uint32, uint32);
        REGISTER(setStrAtIndex, 3, void, uint32, uint32, BSFixedString);

        REGISTER(getFlt, 2, float, uint32, BSFixedString);
        REGISTER(getInt, 2, int32, uint32, BSFixedString);
        REGISTER(getStr, 2, BSFixedString, uint32, BSFixedString);
        REGISTER(getObject, 2, int32, uint32, BSFixedString);

        REGISTER(fltAtIndex, 2, float, uint32, uint32);
        REGISTER(strAtIndex, 2, BSFixedString, uint32, uint32);
        REGISTER(intAtIndex, 2, int32, uint32, uint32);
        REGISTER(objectAtIndex, 2, uint32, uint32, uint32);

        REGISTER2("setFlt", setNum<float>, 3, void, uint32, BSFixedString, float);
        REGISTER2("setInt", setNum<int32>, 3, void, uint32, BSFixedString, int32);
        REGISTER2("setStr", setStr, 3, void, uint32, BSFixedString, BSFixedString);
        REGISTER(setObjectForKey, 3, void, uint32, BSFixedString, uint32);

        REGISTER2("count", JValueArrayCount, 1, uint32, uint32);

        _DMESSAGE("funcs registered");

        return true;
    }




    //template<> bool array<Float32>::registerFuncs(VMClassRegistry* registry);
}
