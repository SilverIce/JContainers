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

    typedef struct json_t * json_ref;
    typedef std::unique_ptr<json_t, decltype(json_decref)> json_unique_ref;

    class json_deserializer {
        tes_context& _context;

        typedef std::vector<std::pair<object_base*, json_ref> > objects_to_fill;

        objects_to_fill _toFill;

        explicit json_deserializer(tes_context& context) : _context(context) {}

    public:

        static auto json_from_file(const char *path) -> decltype( make_unique_ptr((json_ref)nullptr, json_decref)) {
            json_error_t error; //  TODO: output error
            json_ref ref = json_load_file(path, 0, &error);
            return make_unique_ptr(ref, json_decref);
        }

        static auto json_from_data(const char *data) -> decltype( make_unique_ptr((json_ref)nullptr, json_decref)) {
            json_error_t error; //  TODO: output error
            json_ref ref = json_loads(data, 0, &error);
            return make_unique_ptr(ref, json_decref);
        }

        static object_base* object_from_json_data(tes_context& context, const char *data) {
            auto json = json_from_data(data);
            return json_deserializer(context)._object_from_json( json.get() );
        }

        static object_base* object_from_json(tes_context& context, json_ref ref) {
            return json_deserializer(context)._object_from_json( ref );
        }

        static object_base* object_from_file(tes_context& context, const char *path) {
            auto json = json_from_file(path);
            return json_deserializer(context)._object_from_json( json.get() );
        }

    private:

        object_base* _object_from_json(json_ref ref) {
            if (!ref) {
                return nullptr;
            }

            auto& root = make_placeholder(ref);

            while (_toFill.empty() == false) {
                objects_to_fill toFill;
                toFill.swap(_toFill);

                for (auto& pair : toFill) {
                    fill_object(*pair.first, pair.second);
                }
            }

            return &root;
        }

        void fill_object(object_base& object, json_ref val) {
            object_lock lock(object);

            if (array *arr = object.as<array>()) {
                /* array is a JSON array */
                size_t index = 0;
                json_t *value = nullptr;

                json_array_foreach(val, index, value) {
                    arr->u_push(make_item(value));
                }
            }
            else if (map *cnt = object.as<map>()) {
                const char *key;
                json_t *value;

                json_object_foreach(val, key, value) {
                    cnt->u_setValueForKey(key, make_item(value));
                }
            }
            else if (form_map *cnt = object.as<form_map>()) {
                const char *key;
                json_t *value;

                json_object_foreach(val, key, value) {
                    auto fkey = form_handling::from_string(key);
                    if (fkey) {
                        cnt->u_setValueForKey(*fkey, make_item(value));
                    }
                }
            }
        }

        object_base& make_placeholder(json_ref val) {
            object_base *object = nullptr;
            auto type = json_typeof(val);

            if (type == JSON_ARRAY) {
                object = array::object(_context);
            }
            else if (type == JSON_OBJECT) {
                if (!json_object_get(val, form_handling::kFormData)) {
                    object = map::object(_context);
                } else {
                    object = form_map::object(_context);
                }
            }

            assert(object);
            _toFill.push_back(std::make_pair(object, val));
            return *object;
        }

        Item make_item(json_ref val) {
            Item item;

            auto type = json_typeof(val);

            if (type == JSON_ARRAY || type == JSON_OBJECT) {
                item = &make_placeholder(val);
            }
            else if (type == JSON_STRING) {

                /*  having dilemma here:
                    if string looks likes form-string and plugin name can't be resolved:
                    a. lost info and convert it to FormZero
                    b. save info and convert it to string
                */

                auto string = json_string_value(val);
                if (!form_handling::is_form_string(string)) {
                    item = string;
                } else {
                    item = form_handling::from_string(string).get_value_or(FormZero);
                }
            }
            else if (type == JSON_INTEGER) {
                item = (int)json_integer_value(val);
            }
            else if (type == JSON_REAL) {
                item = json_real_value(val);
            }
            else if (type == JSON_TRUE || type || JSON_FALSE) {
                item = json_boolean_value(val);
            }

            return item;
        }
    };

    

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


