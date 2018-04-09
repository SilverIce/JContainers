#pragma once

#include <set>
#include <vector>
#include <map>
#include <jansson.h>
#include <memory>

#include "boost/filesystem/path.hpp"
#include "boost_extras.h"

#include "forms/form_handling.h"
#include "collections/collections.h"
#include "collections/access.h"

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
    typedef decltype(make_unique_ptr((json_t*)nullptr, json_decref)) json_unique_ref;

    namespace reference_serialization {
        // In current state it does not YET requires UTF-8 support

        const char prefix[] = "__reference|";
        const char separator = '|';

        inline bool is_special_string(const char *str) {
            char spec[] = "__";
            return str && strncmp(str, spec, sizeof spec - 1) == 0;
        }

        inline bool is_reference(const char *str) {
            return str && strncmp(str, prefix, sizeof prefix - 1) == 0;
        }

        inline const char* extract_path(const char *str) {
            return is_reference(str) ? (str + sizeof(prefix) - 1) : nullptr;
        }
    }

    namespace json_object_serialization_consts {

        static const char * kMetaInfo = "__metaInfo";
        static const char * kMetaInfoLegacy = "__formData";
        static const char * kTypeName = "typeName";

        template<class T> inline const char* type2name();

        template<> inline const char* type2name<form_map>() { return "JFormMap"; }
        template<> inline const char* type2name<integer_map>() { return "JIntMap"; }

        template<class T> inline void put_metainfo(json_t* object) {
            auto metaInfo = json_object();
            json_object_set_new(metaInfo, kTypeName, json_string(type2name<T>()));
            json_object_set_new(object, kMetaInfo, metaInfo);
        }
    }

    class json_deserializer {
        typedef std::vector<std::pair<object_base*, json_ref> > objects_to_fill;

        typedef ca::key_variant key_variant;
        // path - <container, key> pairs relationship
        typedef std::map<std::string, std::vector<std::pair<object_base*, key_variant > > > key_info_map;

        tes_context& _context;
        objects_to_fill _toFill;
        key_info_map _toResolve;

        explicit json_deserializer(tes_context& context) : _context(context) {}

    public:

        static json_unique_ref json_from_file(const char *path) {
            json_ref ref = nullptr;
            if (path) {
                json_error_t error; //  TODO: output error
                auto file = make_unique_ptr(fopen(path, "rb"), fclose);
                auto cb = [](void *buffer, size_t buflen, void *data) -> size_t {
                    return data ? fread(buffer, 1, buflen, reinterpret_cast<FILE*>(data)) : 0;
                };
                ref = json_load_callback(cb, file.get(), 0, &error);

                if (!ref) {
                    JC_LOG_ERROR("Can't parse JSON file at '%s' at line %u:%u - %s",
                        path, error.line, error.column, error.text);
                }
            }
            return make_unique_ptr(ref, json_decref);
        }

        static json_unique_ref json_from_data(const char *data) {
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

        static object_base* object_from_file(tes_context& context, const boost::filesystem::path& path) {
            return object_from_file(context, path.generic_string().c_str());
        }

    private:

        object_base* _object_from_json(json_ref ref) {
            if (!ref) {
                return nullptr;
            }

            auto root = make_placeholder(ref);
            if (!root) {
                return nullptr;
            }

            while (_toFill.empty() == false) {
                objects_to_fill toFill;
                toFill.swap(_toFill);

                for (auto& pair : toFill) {
                    fill_object(*pair.first, pair.second);
                }
            }

            resolve_references(*root);

            return root;
        }

        void resolve_references(object_base& root) {

            for (const auto& pair : _toResolve) {
                auto& path = pair.first;
                object_base *resolvedObject = nullptr;

                if (path.empty() == false) {
                    ca::visit_value(root, path.c_str(), ca::constant, [&resolvedObject](item& itm) {
                        resolvedObject = itm.object();
                    });
                }
                else { // special case "__reference|"
                    resolvedObject = &root;
                }

                if (!resolvedObject) {
                    continue;
                }

                for (auto& obj2Key : pair.second) {
                    object_lock l(obj2Key.first);
                    ca::u_assign_value(*obj2Key.first, obj2Key.second, resolvedObject);
                }
            }

        }

        void fill_object(object_base& object, json_ref val) {

            struct helper {
                json_deserializer* self;
                json_ref val;
                void operator()(array& arr) {
                    size_t index = 0;
                    json_t *value = nullptr;
                    json_array_foreach(val, index, value) {
                        arr.u_push(self->make_item(value, arr, index));
                    }
                }
                void operator()(map& cnt) {
                    const char *key;
                    json_t *value;
                    json_object_foreach(val, key, value) {
                        cnt.u_set(key, self->make_item(value, cnt, key));
                    }
                }
                void operator () (form_map& cnt) 
                {
                    const char* key;
                    json_t* value;
                    json_object_foreach (val, key, value) 
                        if (auto fkey = forms::string_to_form (key))
                        {
                            form_ref weak_key = make_weak_form_id (*fkey, self->_context);
                            cnt.u_set (weak_key, self->make_item (value, cnt, weak_key));
                        }
                }
                void operator()(integer_map& cnt) {
                    const char *key;
                    json_t *value;
                    std::string temp;
                    json_object_foreach(val, key, value) {
                        try {
                            temp = key;
                            int32_t intKey = std::stoi(temp, nullptr, 0);
                            cnt.u_container()[intKey] = self->make_item(value, cnt, intKey);
                        }
                        catch (const std::invalid_argument&) {}
                        catch (const std::out_of_range&) {}
                    }
                }
            };

            object_lock lock(object);
            perform_on_object(object, helper{ this, val });
        }

        object_base* make_placeholder(json_ref val) {
            object_base *object = nullptr;
            auto type = json_typeof(val);

            if (type == JSON_ARRAY) {
                object = &array::object(_context);
            }
            else if (type == JSON_OBJECT) {
                namespace jsc = json_object_serialization_consts;

                json_t* metaInfo = json_object_get(val, jsc::kMetaInfo);
                if (!metaInfo) { // legacy key
                    metaInfo = json_object_get(val, jsc::kMetaInfoLegacy);
                }

                if (json_is_null(metaInfo)) { // legacy format
                    object = &form_map::object(_context);
                }
                else if (json_is_object(metaInfo)) { // handle metaInfo
                    auto typeName = json_string_value(json_object_get(metaInfo, jsc::kTypeName));
                    if (strcmp(jsc::type2name<form_map>(), typeName) == 0) {
                        object = &form_map::object(_context);
                    }
                    else if (strcmp(jsc::type2name<integer_map>(), typeName) == 0) {
                        object = &integer_map::object(_context);
                    }
                }
                else {
                    object = &map::object(_context);
                }

                if (metaInfo) {
                    json_object_del(val, jsc::kMetaInfo);
                    json_object_del(val, jsc::kMetaInfoLegacy);
                }
            }

            if (object) {
                _toFill.push_back(std::make_pair(object, val));
            }

            return object;
        }

        template<class K>
        bool schedule_ref_resolving(const char *ref_string, object_base& container, const K& item_key) {
            const char* path = reference_serialization::extract_path(ref_string);

            if (!path) {
                return false;
            }

            _toResolve[path].push_back( std::make_pair(&container, item_key) );
            return true;
        }

        template<class K>
        item make_item(json_ref val, object_base& container, const K& item_key) {
            item item;

            switch (json_typeof(val))
            {
            case JSON_OBJECT:
            case JSON_ARRAY:
                item = make_placeholder(val);
                break;
            case JSON_STRING:{
                auto string = json_string_value(val);

                if (!reference_serialization::is_special_string(string)) {
                    item = string;
                } else {
                    if (forms::is_form_string(string)) {
                        /*  having dilemma here:
                            if the string looks like form-string and plugin name can't be resolved:
                            a. lost info and convert it to FormZero
                            b. save info and convert it to string
                        */
                        item = make_weak_form_id (forms::string_to_form (string).value_or (FormId::Zero), _context);
                    }
                    else if (schedule_ref_resolving(string, container, item_key)) { // otherwise it's reference string?
                        ;
                    }
                    else {  // otherwise it's just a string, although it starts with "__"
                        item = string;
                    }
                }

            }
                break;
            case JSON_INTEGER:
                item = (int)json_integer_value(val);
                break;
            case JSON_REAL:
                item = json_real_value(val);
                break;
            case JSON_TRUE:
            case JSON_FALSE:
                item = json_is_true(val);
                break;
            case JSON_NULL:
            default:
                break;
            }

            return item;
        }
    };


    class json_serializer {

        using object_cref = std::reference_wrapper<const object_base>;
        struct object_cref_cmp {
            bool operator()(const object_cref& l, const object_cref& r) const {
                return &l.get() < &r.get();
            }
        };

        typedef std::set<object_cref, object_cref_cmp> collection_set;
        typedef std::vector<std::pair<object_cref, json_ref> > objects_to_fill;

        const object_base& _root;
        collection_set _serializedObjects;
        objects_to_fill _toFill;

        typedef ca::key_variant key_variant;
        // contained-object to <container-owner, key> relation
        typedef std::map<object_cref, std::pair<object_cref, key_variant>, object_cref_cmp> key_info_map;
        key_info_map _keyInfo;

        explicit json_serializer(const object_base& root) : _root(root) {}

    public:

        static json_unique_ref create_json_value(const object_base &root) {
            return make_unique_ptr( json_serializer(root)._write_json(root), &json_decref);
        }

        static auto create_json_data(const object_base &root) -> decltype(make_unique_ptr((char*)nullptr, free)) {

            auto jvalue = create_json_value(root);

            return make_unique_ptr(
                jvalue ? json_dumps(jvalue.get(), JSON_INDENT(2)) : nullptr,
                free
            );
        }

    private:

        // writes to json
        json_ref _write_json(const object_base &root) {

            auto root_value = create_placeholder(root);

            while (_toFill.empty() == false) {

                objects_to_fill toFill;
                toFill.swap(_toFill);

                for (auto& pair : toFill) {
                    fill_json_object(pair.first, pair.second);
                }
            }

            return root_value;
        }

        json_ref create_placeholder(const object_base& object) {

            json_ref placeholder = nullptr;
            auto obj_cref = std::cref(object);

            if (_serializedObjects.find(obj_cref) == _serializedObjects.end()) {
                placeholder = object.as<array>() ? json_array() : json_object();
                _toFill.push_back(objects_to_fill::value_type(obj_cref, placeholder));
                _serializedObjects.insert(obj_cref);
            }
            else {
                placeholder = json_string(path_to_object(object).c_str());
            }

            return placeholder;
        }

        void fill_json_object(const object_base& cnt, json_ref object) {
            struct helper {
                json_serializer * self;
                json_ref object;

                void operator () (const array& cnt) {
                    size_t index = 0;
                    for (auto& itm : cnt.u_container()) {
                        self->fill_key_info(itm, cnt, index++);
                        json_array_append_new(object, self->create_value(itm));
                    }
                }
                void operator () (const map& cnt) {
                    for (auto& pair : cnt.u_container()) {
                        self->fill_key_info(pair.second, cnt, pair.first);
                        json_object_set_new(object, pair.first.c_str(), self->create_value(pair.second));
                    }
                }
                void operator () (const form_map& cnt) {
                    json_object_serialization_consts::put_metainfo<form_map>(object);

                    for (auto& pair : cnt.u_container()) {
                        auto key = forms::form_to_string(pair.first.get());
                        if (key) {
                            self->fill_key_info(pair.second, cnt, pair.first);
                            json_object_set_new(object, (*key).c_str(), self->create_value(pair.second));
                        }
                    }
                }
                void operator () (const integer_map& cnt) {

                    json_object_serialization_consts::put_metainfo<integer_map>(object);

                    char key_string[number_to_string_buffer_size] = { '\0' };

                    for (auto& pair : cnt.u_container()) {
                        self->fill_key_info(pair.second, cnt, pair.first);
                        assert(-1 != sprintf_s(key_string, "%d", pair.first));
                        json_object_set_new(object, key_string, self->create_value(pair.second));
                    }
                }
            };

            object_lock lock(cnt);
            perform_on_object(cnt, helper{ this, object });
        }

        template<class Key>
        void fill_key_info(const item& value, const object_base& in_object, const Key& key) {
            if (auto obj = value.object()) {
                _keyInfo.emplace(key_info_map::value_type{ std::cref(*obj), { std::cref(in_object), key } });
            }
        }

        json_ref create_value(const item& item) {

            struct item_visitor : boost::static_visitor<json_ref> {

                json_serializer& ser;

                item_visitor(json_serializer& s) : ser(s) {}

                static json_ref null() {
                    return json_null();
                }

                json_ref operator()(const std::string & val) const {
                    return json_stringn(val.c_str(), val.size());
                }

                json_ref operator()(const boost::blank&) const {
                    return null();
                }

                json_ref operator()(const SInt32 & val) const {
                    return json_integer(val);
                }

                json_ref operator()(const item::Real & val) const {
                    return json_real(val);
                }

                json_ref operator()(const form_ref& val) const {
                    auto formStr = forms::form_to_string(val.get());
                    if (formStr) {
                        return (*this)(*formStr);
                    }
                    else {
                        return null();
                    }
                }

                json_ref operator()(const internal_object_ref & val) const {
                    object_base *obj = val.get();
                    return obj ? ser.create_placeholder(*obj) : null();
                }

            } item_visitor = { *this };

            json_ref val = item.var().apply_visitor(item_visitor);
            return val;
        }

        enum  {
            number_to_string_buffer_size = 20,
        };

        std::string path_to_object(const object_base& obj) const {


            struct path_appender : boost::static_visitor<> {
                std::string& p;

                path_appender(std::string& path) : p(path) {}

                void operator()(const std::string & key) const {
                    p.append(".");
                    p.append(key);
                }

                void operator()(const int32_t& idx) const {
                    char data[number_to_string_buffer_size] = { '\0' };
                    assert(-1 != sprintf_s(data, "[%d]", idx));
                    p.append(data);
                }

                void operator()(const form_ref& fid) const {
                    p.append("[");
                    p.append(*forms::form_to_string(fid.get()));
                    p.append("]");
                }
            };

            std::deque<std::reference_wrapper<const key_variant>> keys;
            auto child = std::cref(obj);

            while (&child.get() != &_root) {

                auto itr = _keyInfo.find(child);

                if (itr != _keyInfo.end()) {
                    auto& pair = itr->second;
                    child = pair.first;
                    keys.push_front(pair.second);
                }
                else {
                    break;
                }
            }

            std::string path{ reference_serialization::prefix };
            path_appender pa = { path };

            for (auto& key_var : keys) {
                boost::apply_visitor(pa, key_var.get());
            }

            return path;
        }

    };

}


