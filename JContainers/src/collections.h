#pragma once

#include <vector>
#include <string>
#include <assert.h>

#include <boost/serialization/split_member.hpp>
#include <boost/optional.hpp>

#include "common/ITypes.h"
#include "common/IDebugLog.h"
#include "skse/GameForms.h"

//#include "tes_context.h"
#include "object/object_base.h"
#include "skse.h"

#include "collection_item.h"
#include "collection_types.h"

namespace collections {

	class tes_context;

    template<class T>
    class collection_base : public object_base
    {
        collection_base(const collection_base&);
        collection_base& operator=(const collection_base&);

    protected:

        //static_assert(std::is_base_of<collection_base<T>, T>::value, "");

        explicit collection_base() : object_base((CollectionType)T::TypeId) {}

    public:

        // just for convence to not use static_cast's
        object_base& base() { return *this; }
        const object_base& base() const { return *this; }

        typedef typename object_stack_ref_template<T> ref;
        typedef typename object_stack_ref_template<const T> cref;

        static T& make(tes_context& context /*= tes_context::instance()*/) {
            auto& obj = *new T();
            obj.set_context(context);
            obj._registerSelf();
            return obj;
        }

        template<class Init>
        static T& _makeWithInitializer(Init& init, tes_context& context /*= tes_context::instance()*/) {
            auto& obj = *new T();
            obj.set_context(context);
            init(obj);
            obj._registerSelf();
            return obj;
        }

        static T& object(tes_context& context /*= tes_context::instance()*/) {
            return make(context);
        }

        template<class Init>
        static T& objectWithInitializer(Init& init, tes_context& context /*= tes_context::instance()*/) {
            return _makeWithInitializer(init, context);
        }
    };

    
    template<class R, class F>
    inline R perform_on_object_and_return(object_base & container, F& func) {
        switch (container.type()) {
        case array::TypeId:
            return func(container.as_link<array>());
        case map::TypeId:
            return func(container.as_link<map>());
        case form_map::TypeId:
            return func(container.as_link<form_map>());
        case integer_map::TypeId:
            return func(container.as_link<integer_map>());
        default:
            assert(false);
            noreturn_func();
            break;
        }
    }

    template<class R, class F>
    inline R perform_on_object_and_return(const object_base & container, F& func) {
        return perform_on_object_and_return<R>(const_cast<object_base&>(container), func);
    }

    template<class F>
    inline void perform_on_object(object_base & container, F& func) {
        switch (container.type()) {
        case array::TypeId:
            func(container.as_link<array>());
            break;
        case map::TypeId:
            func(container.as_link<map>());
            break;
        case form_map::TypeId:
            func(container.as_link<form_map>());
            break;
        case integer_map::TypeId:
            func(container.as_link<integer_map>());
            break;
        default:
            assert(false);
            break;
        }
    }

    template<class F>
    inline void perform_on_object(const object_base & container, F& func) {
        perform_on_object(const_cast<object_base&>(container), func);
    }

    class array;
    class map;
    class object_base;

    class array : public collection_base< array >
    {
        array(const array&);
        array& operator=(const array&);

    public:

        array() {}

        enum {
            TypeId = CollectionType::Array,
        };

        typedef SInt32 Index;

        typedef std::vector<Item> container_type;
        typedef container_type::iterator iterator;
        typedef container_type::reverse_iterator reverse_iterator;

        container_type _array;

        container_type& u_container() {
            return _array;
        }

        const container_type& u_container() const {
            return _array;
        }

        container_type container_copy() const {
            object_lock g(this);
            return _array;
        }

        template<class T> void push(T&& item) {
            object_lock g(this);
            u_push(item);
        }

        template<class T> void u_push(T&& item) {
            _array.emplace_back(std::forward<T>(item));
        }

        void u_clear() override {
            _array.clear();
        }

        SInt32 u_count() const override {
            return _array.size();
        }

        void u_nullifyObjects() override;

        void u_visit_referenced_objects(const std::function<void(object_base&)>& visitor) override {
            for (auto& item : _array) {
                if (auto obj = item.object()) {
                    visitor(*obj);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////

        boost::optional<int32_t> u_convertIndex(int32_t pyIndex) const {
            int32_t count = (int32_t)_array.size();
            int32_t index = (pyIndex >= 0 ? pyIndex : (count + pyIndex));
            return{ index >= 0 && index < count, index };
        }

        Item* u_getItem(int32_t index) {
            auto idx = u_convertIndex(index);
            return idx ? &_array[*idx] : nullptr;
        }

        void setItem(int32_t index, const Item& itm) {
            object_lock g(this);
            auto idx = u_convertIndex(index);
            if (idx) {
                _array[*idx] = itm;
            }
        }

        Item& operator [] (int32_t index) { return const_cast<Item&>(const_cast<const array*>(this)->operator[](index)); }
        const Item& operator [] (int32_t index) const {
            auto idx = u_convertIndex(index);
            assert(idx);
            return _array[*idx];
        }

        Item getItem(int32_t index) {
            object_lock lock(this);
            auto itm = u_getItem(index);
            return itm ? *itm : Item();
        }

        iterator begin() { return _array.begin();}
        iterator end() { return _array.end(); }

        reverse_iterator rbegin() { return _array.rbegin();}
        reverse_iterator rend() { return _array.rend(); }


        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };

    template<class RealType, class ContainerType>
    class basic_map_collection : public collection_base< RealType > {
    public:
        typedef ContainerType container_type;
        typedef typename ContainerType::key_type key_type;
    protected:
        ContainerType cnt;

    public:

        const container_type& u_container() const {
            return cnt;
        }

        container_type& u_container() {
            return cnt;
        }

        container_type container_copy() {
            object_lock g(this);
            return cnt;
        }

        Item findOrDef(const key_type& key) {
            object_lock g(this);
            auto result = u_find(key);
            return result ? *result : Item();
        }

        const Item* u_find(const key_type& key) const {
            auto itr = cnt.find(key);
            return itr != cnt.end() ? &(itr->second) : nullptr;
        }

        Item* u_find(const key_type& key) { return const_cast<Item*>( const_cast<const basic_map_collection*>(this)->u_find(key) ); }

        bool erase(const key_type& key) {
            object_lock g(this);
            return u_erase(key);
        }

        bool u_erase(const key_type& key) {
            auto itr = cnt.find(key);
            return itr != cnt.end() ? (cnt.erase(itr), true) : false;
        }

        void u_clear() override {
            cnt.clear();
        }

        template<class T> void u_setValueForKey(const key_type& key, T&& value) {
            cnt[key] = std::forward<T>(value);
        }

        template<class T>void setValueForKey(const key_type& key, T&& value) {
            object_lock g(this);
            u_setValueForKey(key, value);
        }

        SInt32 u_count() const override {
            return cnt.size();
        }

        Item& operator [] (const key_type& key) {
            return const_cast<Item&>(const_cast<const basic_map_collection&>(*this)[key]);
        }

        const Item& operator [] (const key_type& key) const {
            auto itm = u_find(key);
            assert(itm);
            return *itm;
        }
        
        void u_visit_referenced_objects(const std::function<void(object_base&)>& visitor) override {
            for (auto& pair : u_container()) {
                if (auto obj = pair.second.object()) {
                    visitor(*obj);
                }
            }
        }

        void u_nullifyObjects() override {
            for (auto& pair : u_container()) {
                pair.second.u_nullifyObject();
            }
        }
    };

    struct map_case_insensitive_comp {
        bool operator() (const std::string& lhs, const std::string& rhs) const {
            return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
        }
    };


    class map : public basic_map_collection< map, std::map<std::string, Item, map_case_insensitive_comp > >
    {
    public:
        enum  {
            TypeId = CollectionType::Map,
        };

        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };


    class form_map : public basic_map_collection< form_map, std::map<FormId, Item> >
    {
    public:
        enum  {
            TypeId = CollectionType::FormMap,
        };

        void u_onLoaded() override;

        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };

    class integer_map : public basic_map_collection < integer_map, std::map<int32_t, Item> >
    {
    public:
        enum  {
            TypeId = CollectionType::IntegerMap,
        };

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };
}
