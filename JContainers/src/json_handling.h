#pragma once

#include <set>
#include <vector>
#include <jansson.h>

#include "collections.h"
#include "form_handling.h"

namespace collections {

    inline std::unique_ptr<FILE, decltype(&fclose)> make_unique_file(FILE *file) {
        return std::unique_ptr<FILE, decltype(&fclose)> (file, &fclose);
    }

    template<class T, class D>
    inline std::unique_ptr<T, D> make_unique_ptr(T* data, D destr) {
        return std::unique_ptr<T, D> (data, destr);
    }

    template<class T>
    inline std::unique_ptr<T, std::default_delete<T> > make_unique_ptr(T* data) {
        return std::unique_ptr<T, std::default_delete<T> > ( data, std::default_delete<T>() );
    }


    class json_deserializer {
        tes_context& _context; 
    public:

        explicit json_deserializer(tes_context & context) : _context(context) {}

        object_base * readJSONFile(const char *path) {
            auto cj = cJSONFromFile(path);
            auto res = readCJSON(cj);
            cJSON_Delete(cj);
            return res;
        }

        object_base * readJSONData(const char *text) {
            if (!text) {
                return nullptr;
            }
            auto cj = cJSON_Parse(text);
            auto res = readCJSON(cj);
            cJSON_Delete(cj);
            return res;
        }

        object_base * readCJSON(cJSON *value) {
            if (!value) {
                return nullptr;
            }

            object_base *obj = nullptr;
            if (value->type == cJSON_Array || value->type == cJSON_Object) {
                Item itm = makeItem(value);
                obj = itm.object();
            }

            return obj;
        }

        cJSON * cJSONFromFile(const char *path) {
            if (!path) {
                return nullptr;
            }

            using namespace std;

            auto file = make_unique_file(fopen(path, "r"));
            if (!file.get())
                return nullptr;

            char buffer[1024];
            std::vector<char> bytes;
            while (!ferror(file.get()) && !feof(file.get())) {
                size_t readen = fread(buffer, 1, sizeof(buffer), file.get());
                if (readen > 0) {
                    bytes.insert(bytes.end(), buffer, buffer + readen);
                }
                else  {
                    break;
                }
            }
            bytes.push_back(0);

            return cJSON_Parse(&bytes[0]);
        }

        array * makeArray(cJSON *val) {
            array *ar = array::object(_context);

            int count = cJSON_GetArraySize(val);
            for (int i = 0; i < count; ++i) {
            	ar->u_push(makeItem(cJSON_GetArrayItem(val, i)));
            }

            return ar;
        }

        object_base * makeObject(cJSON *val) {

            object_base *object = nullptr;
            bool isFormMap = cJSON_GetObjectItem(val, form_handling::kFormData) != nullptr;

            if (isFormMap) {
                auto obj = form_map::object(_context);
                object = obj;

                int count = cJSON_GetArraySize(val);
                for (int i = 0; i < count; ++i) {
                    auto itm = cJSON_GetArrayItem(val, i);
                    auto key = form_handling::from_string(itm->string);

                    if (key) {
                        obj->u_setValueForKey(*key, makeItem(itm));
                    }
                }
            } else {

                auto obj = map::object(_context);
                object = obj;

                int count = cJSON_GetArraySize(val);
                for (int i = 0; i < count; ++i) {
                    auto itm = cJSON_GetArrayItem(val, i);
                    obj->u_setValueForKey(itm->string, makeItem(itm));
                }
            }

            return object;
        }

        Item makeItem(cJSON *val) {
            Item item;
            int type = val->type;
            if (type == cJSON_Array) {
                item = makeArray(val);
            } else if (type == cJSON_Object) {
                item = makeObject(val);
            } else if (type == cJSON_String) {

                bool isFormData = strncmp(val->valuestring, form_handling::kFormData, strlen(form_handling::kFormData)) == 0;

                if (!isFormData) {
                    item = val->valuestring;
                } else {
                    item = *form_handling::from_string(val->valuestring);
                }

            } else if (type == cJSON_Number) {
                if (std::floor(val->valuedouble) == val->valuedouble) {
                    item = val->valueint;
                } else {
                    item = val->valuedouble;
                }
            }

            return item;
        }
    };

    
    typedef struct json_t * json_ref;

/*
    class item_serializer : public boost::static_visitor<json_ref>
    {
    public:

        json_ref null() const {
            return * new js_value();
        }

        json_ref operator()(const std::string & val) {
            return * new js_value(val);
        }

        json_ref operator()(boost::blank ) {
            return null();
        }

        // SInt32, Float32, std::string, object_ref, FormId

        json_ref operator()(const SInt32 & val) {
            return * new js_value(val);
        }

        json_ref operator()(const Float32 & val) {
            return * new js_value(val);
        }

        json_ref operator()(const FormId&  val) {
            auto formStr = form_handling::to_string(val);
            if (formStr) {
                return * new js_value(*formStr);
            } else {
                return null();
            }
        }

        json_ref operator()(const object_ref & val) {
            return null();
        }

    };
*/


    class json_serializer : public boost::static_visitor<json_ref> {

        typedef std::set<object_base*> collection_set;
        typedef std::vector<std::pair<object_base*, json_ref> > objects_to_fill;

        collection_set _serializedObjects;
        objects_to_fill _toFill;

        json_serializer() {}

    public:

        static auto create_json_value(object_base &root) -> decltype(make_unique_ptr((json_ref)nullptr, json_decref)) {
            return make_unique_ptr( json_serializer()._write_json(root), &json_decref);
        }

        static auto create_json_data(object_base &root) -> decltype(make_unique_ptr((char*)nullptr, free)) {

            auto jvalue = create_json_value(root);

            return make_unique_ptr(
                jvalue ? json_dumps(jvalue.get(), JSON_INDENT(2)) : nullptr,
                free
                );
        }

    private:

        // writes to json
        json_ref _write_json(object_base &root) {

            auto root_value = create_placeholder(root);

            while (_toFill.empty() == false) {

                objects_to_fill toFill;
                toFill.swap(_toFill);

                for (auto& pair : toFill) {
                    fill_json_object(*pair.first, pair.second);
                }
            }

            return root_value;
        }

        json_ref create_placeholder(object_base& object) {

            json_ref placeholder = nullptr;

            if (object.as<array>()) {
                placeholder = json_array();
            }
            else if (object.as<map>() || object.as<form_map>()) {
                placeholder = json_object();
            }
            assert(placeholder);

            _toFill.push_back( objects_to_fill::value_type(&object, placeholder) );
            return placeholder;
        }

        void fill_json_object(object_base& cnt, json_ref object) {

            object_lock lock(cnt);

            if (cnt.as<array>()) {
                for (auto& itm : cnt.as<array>()->u_container()) {
                    json_array_append_new(object, create_value(itm));
                }
            }
            else if (cnt.as<map>()) {
                for (auto& pair : cnt.as<map>()->u_container()) {
                    json_object_set_new(object, pair.first.c_str(), create_value(pair.second));
                }
            }
            else if (cnt.as<form_map>()) {
                // mark object as form_map container
                json_object_set_new(object, form_handling::kFormData, json_null());

                for (auto& pair : cnt.as<form_map>()->u_container()) {
                    auto key = form_handling::to_string(pair.first);
                    if (key) {
                        json_object_set_new(object, (*key).c_str(), create_value(pair.second));
                    }
                }
            }
        }

        json_ref create_value(const Item& item) {
            json_ref val = item.var().apply_visitor( *this );
            return val;
        }

    public:

        // part of visitor functionality

        static json_ref null() {
            return json_null();
        }

        json_ref operator()(const std::string & val) const {
            return json_string(val.c_str());
        }

        json_ref operator()(const boost::blank& ) const {
            return null();
        }

        // SInt32, Float32, std::string, object_ref, FormId

        json_ref operator()(const SInt32 & val) const {
            return json_integer(val);
        }

        json_ref operator()(const Float32 & val) const {
            return json_real(val);
        }

        json_ref operator()(const FormId&  val) const {
            auto formStr = form_handling::to_string(val);
            if (formStr) {
                return (*this)(*formStr);
            } else {
                return null();
            }
        }

        json_ref operator()(const object_ref & val) {
            object_base *obj = val.get();

            if (!obj) {
                return null();
            }

            if (_serializedObjects.find(obj) != _serializedObjects.end()) {
                return json_string("<can not serialize object twice>");
            }

            _serializedObjects.insert(obj);

            json_ref node = create_placeholder(*obj);
            return node;
        }
    };

}


