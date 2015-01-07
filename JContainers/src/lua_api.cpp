#pragma once

//#include "lua_stuff.h"

#include <utility>
#include <string>
#include <boost/optional.hpp>
#include "boost/filesystem/path.hpp"
#include <sstream>

#include "gtest.h"
#include "util.h"

#include "reflection.h"
#include "lua_basics.h"

#include "jcontainers_constants.h"
#include "collections.h"
#include "tes_context.h"
#include "collection_functions.h"

namespace {

    using namespace collections;

#   define cexport extern "C" __declspec(dllexport)

    namespace { // a trick to shut-up compiler
#   include "Data/SKSE/Plugins/JCData/InternalLuaScripts/api_for_lua.h"
    }

    cexport const char* JC_hello() {
        return "lua, are you there?";
    }

    cexport void* JC_get_c_function(const char *functionName, const char * className) {
        using namespace reflection;

        c_function functionPtr = nullptr;
        auto& db = class_database();
        auto itr = db.find(className);
        if (itr != db.end()) {
            auto& cls = itr->second;

            if (const function_info* fInfo = cls.find_function(functionName)) {
                functionPtr = fInfo->c_func;
            }
        }

        return functionPtr;
    }

    JCToLuaValue JCToLuaValue_None() {
        return{ item_type::no_item, { 0 }, 0 };
    }

    cexport void JCToLuaValue_free(JCToLuaValue* v) {
        if (v && v->type == item_type::string) {
            free((void *)v->string);
        }
    }

    cexport void CString_free(CString *str) {
        if (str) {
            free((void *)str->str);
        }
    }

    CString CString_None() {
        return{ nullptr, 0 };
    }

    CString CString_copy(const char *origin, size_t length = -1) {
        if (!origin) {
            return CString_None();
        }

        length = length != -1 ? length : strlen(origin);
        char *string = (char *)calloc(length + 1, sizeof(char));
        strcpy(string, origin);
        return{ string, length };
    }


    CString CString_copy(const std::string& origin) {
        return CString_copy(origin.c_str(), origin.size());
    }

    JCToLuaValue JCToLuaValue_fromItem(const Item& itm) {
        struct t : public boost::static_visitor < > {
            JCToLuaValue value;

            void operator ()(const std::string& str) {
                value.string = CString_copy(str.c_str(), str.size()).str;
                value.stringLength = str.size();
            }

            void operator ()(const Item::Real& val) {
                value.real = val;
            }

            void operator ()(const SInt32& val) {
                value.integer = val;
            }

            void operator ()(const boost::blank&) {}

            void operator ()(const collections::internal_object_ref& val) {
                value.object = { val.get() };
            }

            void operator ()(const FormId& val) {
                value.form = { val };
            }

        } converter;

        converter.value.type = itm.type();
        itm.var().apply_visitor(converter);
        return converter.value;
    }

    void JCValue_fillItem(const JCValue *v, Item& itm) {
        switch (v ? v->type : item_type::no_item) {
        case item_type::form:
            itm = (FormId)v->form.___id;
            break;
        case item_type::integer:
            itm = v->integer;
            break;
        case item_type::real:
            itm = v->real;
            break;
        case item_type::object:
            itm = (object_base *)v->object;
            break;
        case item_type::string:
            itm = v->string;
            break;
        case item_type::no_item:
        case item_type::none:
            itm = boost::blank();
            break;
        default:
            assert(false);
            break;
        }
    }

    template<class JCV> std::string JCValue_toString(const JCV *v) {
        std::stringstream str;
        if (!v) {
            str << "nil";
        }
        else {
            switch (v->type)
            {
            case collections::no_item:
                str << "no_item";
                break;
            case collections::none:
                str << "none";
                break;
            case collections::integer:
                str << "integer " << v->integer;
                break;
            case collections::real:
                str << "real " << v->real;
                break;
            case collections::form:
                str << "form " << v->form;
                break;
            case collections::object:
                str << "object " << v->object;
                break;
            case collections::string:
                str << "string " << (const char*)v->string;
                break;
            default:
                assert(false);
                break;
            }
        }

        return str.str();
    }
    template<class JCV> std::string JCValue_toString(const JCV &v) { return JCValue_toString(&v); }

    cexport handle JValue_retain(object_base* obj) { return (obj ? obj->stack_retain(), obj : nullptr); }
    cexport handle JValue_release(object_base* obj) { return (obj ? obj->stack_release(), nullptr : nullptr); }
    cexport collections::CollectionType JValue_typeId(object_base* obj) { return (obj ? obj->_type : CollectionTypeNone); }

    cexport JCToLuaValue JArray_getValue(array* obj, index key) {
        JCToLuaValue v(JCToLuaValue_None());
        array_functions::doReadOp(obj, key, [=, &v](index idx) {
            v = JCToLuaValue_fromItem(obj->u_container()[idx]);
        });
        //std::cout << "value returned: " << JCValue_toString(v) << std::endl;
        return v;
    }

    cexport void JArray_setValue(array* obj, index key, const JCValue* val) {
        array_functions::doReadOp(obj, key, [=](index idx) {
            JCValue_fillItem(val, obj->u_container()[idx]);
        });
        //std::cout << "value assigned: " << JCValue_toString(val) << std::endl;
    }

    cexport void JArray_insert(array* obj, const JCValue* val, index key) {
        array_functions::doWriteOp(obj, key, [=](index idx) {
            auto& cnt = obj->u_container();
            JCValue_fillItem(val, *cnt.insert(cnt.begin() + idx, Item()));
        });
        //std::cout << "value assigned: " << JCValue_toString(val) << std::endl;
    }

    //////////////////////////////////////////////////////////////////////////
    cexport CString JMap_nextKey(const map *obj, cstring lastKey) {
        CString next = CString_None();
        map_functions::nextKey(obj, lastKey, [&](const std::string& key) { next = CString_copy(key); });
        return next;
    }

    cexport void JMap_setValue(map *obj, cstring key, const JCValue* val) {
        map_functions::doWriteOp(obj, key, [val](Item& itm) { JCValue_fillItem(val, itm); });
    }

    cexport JCToLuaValue JMap_getValue(map *obj, cstring key) {
        return map_functions::doReadOpR(obj, key, JCToLuaValue_None(), [](Item& itm) { return JCToLuaValue_fromItem(itm); });
    }
    //////////////////////////////////////////////////////////////////////////

    static_assert(sizeof FormId == sizeof CForm, "");

    cexport FormId JFormMap_nextKey(const form_map *obj, FormId lastKey) {
        FormId next = FormZero;
        formmap_functions::nextKey(obj, lastKey, [&](const FormId& key) { next = key; });
        return next;
    }

    cexport void JFormMap_setValue(form_map *obj, FormId key, const JCValue* val) {
        formmap_functions::doWriteOp(obj, key, [val](Item& itm) { JCValue_fillItem(val, itm); });
    }

    cexport JCToLuaValue JFormMap_getValue(form_map *obj, FormId key) {
        return formmap_functions::doReadOpR(obj, key, JCToLuaValue_None(), [](Item& itm) { return JCToLuaValue_fromItem(itm); });
    }

    cexport handle JDB_instance() {
        return tes_context::instance().database();
    }

}

namespace collections {

    namespace {

        void printLuaError(lua_State *l) {
            if (lua_gettop(l) > 0 && lua_isstring(l, -1)) {
                const char *str = lua_tostring(l, -1);
                _MESSAGE("Lua error: %s", str ? str : "");
                //skse::console_print(str);
            }
        }

        int LuaErrorHandler(lua_State *l) {
            
            int message = lua_gettop(l);//1
            printLuaError(l);

            lua_getglobal(l, "debug");//2
            lua_pushstring(l, "traceback");
            lua_gettable(l, 2); 
            int function = lua_gettop(l);

            lua_call(l, 0, 1);

            printLuaError(l);

            lua_pushvalue(l, message);
            return 1;
        }

        bool setupLuaContext(lua_State *l) {
            typedef boost::filesystem::path path;

            auto initScriptPath = util::relative_to_dll_path(JC_DATA_FILES "InternalLuaScripts/init.lua");

            lua_pushcfunction(l, LuaErrorHandler);
            int errorHandler = lua_gettop(l);

            if (luaL_loadfile(l, initScriptPath.generic_string().c_str()) != lua::LUA_OK) {
                printLuaError(l);
                return false;
            }

            // push data path, dll path
            lua_pushstring(l, util::relative_to_dll_path(JC_DATA_FILES).generic_string().c_str());
            lua_pushstring(l, util::dll_path().generic_string().c_str());

            if (lua_pcall(l, 2, LUA_MULTRET, errorHandler) != lua::LUA_OK) {
                printLuaError(l);
                return false;
            }

            return true;
        }

        void _setupLuaContext(lua_State *l) { setupLuaContext(l); }

        LUA_CONTEXT_MODIFIER(_setupLuaContext);

        boost::optional<Item> eval_lua_function_for_test(lua_State *L, object_base *object, const char *lua_string) {

            lua_pushcfunction(L, LuaErrorHandler);
            int errorHandler = lua_gettop(L);

            lua_getglobal(L, "JC_compileAndRun");
            lua_pushstring(L, lua_string);
            lua_pushlightuserdata(L, object);

            if (lua_pcall(L, 2, 1, errorHandler) != lua::LUA_OK) {
                return boost::none;
            }
            else {
                Item result;
                JCValue *val = (JCValue *)lua_topointer(L, -1);
                JCValue_fillItem(val, result);
                return result;
            }
        }

        TEST_F(lua::fixture, Lua, evalLua_imitation)
        {
            EXPECT_TRUE(setupLuaContext(l));

/*
            auto result = eval_lua_function_for_test(l, &array::object(tes_context::instance()),
                "assert(jobject);"
                "local obj = JArray.objectWithArray {1,2,3,4,9};"
                "return jc.filter(obj, function(x) return x < 3.5 end);"
                );

            EXPECT_TRUE(result);

            auto obj = result->object();
            EXPECT_NOT_NIL(obj);
            auto filtered = result->object()->as<array>();
            EXPECT_NOT_NIL(filtered);
            EXPECT_TRUE(filtered->s_count() == 3);*/

            auto testTransporting = [&](const char *str) { return eval_lua_function_for_test(l, nullptr, str); };

            EXPECT_TRUE(testTransporting("return 10")->intValue() == 10);
            EXPECT_TRUE(testTransporting("return 'die'")->strValue() == std::string("die"));
            EXPECT_TRUE(testTransporting("return Form(20)")->formId() == FormId(20));


            auto db = tes_context::instance().database();

            EXPECT_TRUE(testTransporting("return JDB")->object() == db);

            db->setValueForKey("test", Item(10));
            EXPECT_TRUE(db->find("test").intValue() == 10);

            auto result = eval_lua_function_for_test(l, nullptr,
                "for k,v in pairs(JDB) do print('JDB.test', k, v) end;"
                "assert(JDB.test == 10);"
                "return JDB.test"
                );
            EXPECT_TRUE(result.is_initialized());
            EXPECT_TRUE(result->intValue() == 10);

        }
    }

    boost::optional<Item> eval_lua_function(object_base *object, const char *lua_string) {
        return eval_lua_function_for_test(lua::context::instance().state(), object, lua_string);
    }
}
