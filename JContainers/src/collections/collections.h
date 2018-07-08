#pragma once

#include <vector>
#include <string>
#include <assert.h>

#include <boost/serialization/split_member.hpp>
#include <boost/optional.hpp>

#include "common/ITypes.h"
#include "common/IDebugLog.h"
#include "skse/GameForms.h"
#include "skse/skse.h"

#include "object/object_base.h"

#include "collections/item.h"

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

        // just for convinience - to not use static_cast's
        object_base& base() { return *this; }
        const object_base& base() const { return *this; }

        typedef typename object_stack_ref_template<T> ref;
        typedef typename object_stack_ref_template<const T> cref;

        static T& make(object_context& context /*= tes_context::instance()*/) {
            auto& obj = *new T();
            obj.set_context(context);
            obj._registerSelf();
            return obj;
        }

        template<class Init>
        static T& _makeWithInitializer(Init& init, object_context& context /*= tes_context::instance()*/) {
            auto& obj = *new T();
            obj.set_context(context);
            init(obj);
            obj._registerSelf();
            return obj;
        }

        static T& object(object_context& context /*= tes_context::instance()*/) {
            return make(context);
        }

        template<class Init>
        static T& objectWithInitializer(Init& init, object_context& context /*= tes_context::instance()*/) {
            return _makeWithInitializer(init, context);
        }
    };

    template<class R, class Collection, class F, class ...Args>
    inline R perform_on_object_and_return(Collection& container, F&& func, Args&&... args) {
        switch (container.type()) {
        case array::TypeId:
            return func(container.as_link<array>(), std::forward<Args>(args)...);
        case map::TypeId:
            return func(container.as_link<map>(), std::forward<Args>(args)...);
        case form_map::TypeId:
            return func(container.as_link<form_map>(), std::forward<Args>(args)...);
        case integer_map::TypeId:
            return func(container.as_link<integer_map>(), std::forward<Args>(args)...);
        default:
            assert(false);
            noreturn_func();
            break;
        }
    }

    template<class F, class Collection, class ...Args>
    inline void perform_on_object(Collection& container, F&& func, Args&&... args) {
        switch (container.type()) {
        case array::TypeId:
            func(container.as_link<array>(), std::forward<Args>(args)...);
            break;
        case map::TypeId:
            func(container.as_link<map>(), std::forward<Args>(args)...);
            break;
        case form_map::TypeId:
            func(container.as_link<form_map>(), std::forward<Args>(args)...);
            break;
        case integer_map::TypeId:
            func(container.as_link<integer_map>(), std::forward<Args>(args)...);
            break;
        default:
            assert(false);
            break;
        }
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

        typedef std::vector<item> container_type;
        typedef int32_t key_type;
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
            u_push(std::forward<T>(item));
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

        const item* u_get(int32_t index) const {
            auto idx = u_convertIndex(index);
            return idx ? &_array[*idx] : nullptr;
        }

        item* u_get(int32_t index) {
            return const_cast<item*>( const_cast<const array*>(this)->u_get(index) );
        }

        bool u_erase(int32_t index) {
            auto idx = u_convertIndex(index);
            if (idx) {
                _array.erase(_array.begin() + *idx);
                return true;
            }
            return false;
        }

        template<class T>
        item* u_set(int32_t index, T&& itm) {
            auto idx = u_convertIndex(index);
            if (idx) {
                return &(_array[*idx] = std::forward<T>(itm));
            }
            return nullptr;
        }

        template<class T>
        void set(int32_t index, T&& itm) {
            object_lock g(this);
            u_set(index, std::forward<T>(itm));
        }

        template<class T>
        static boost::optional<T> _opt_from_pointer(const T* t) {
            return t ? boost::optional<T>(*t) : boost::none;
        }

        item& operator [] (int32_t index) { return const_cast<item&>(const_cast<const array*>(this)->operator[](index)); }
        const item& operator [] (int32_t index) const {
            auto idx = u_convertIndex(index);
            assert(idx);
            return _array[*idx];
        }

        boost::optional<item> get_item(int32_t index) const {
            object_lock lock(this);
            return _opt_from_pointer(u_get(index));
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
        using container_type = ContainerType;
        using value_type = typename container_type::value_type;
        using key_type = typename ContainerType::key_type;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;
    protected:
        ContainerType cnt;

        template<class ContainerType>
        static util::choose_iterator<ContainerType> _find(ContainerType& c, const key_type& k) { return c.find(k); }

    public:

        const container_type& u_container() const {
            return cnt;
        }

        container_type& u_container() {
            return cnt;
        }

        container_type container_copy() const {
            object_lock g(this);
            return cnt;
        }

        template<class Key>
        item findOrDef(const Key& key) const {
            object_lock g(this);
            auto result = u_get(key);
            return result ? *result : item();
        }

        template<class Key>
        boost::optional<item> get_item(const Key& key) const {
            object_lock g(this);
            auto result = u_get(key);
            return result ? *result : boost::optional<item>();
        }

        item& u_get_or_create(const key_type& key) {
            return cnt[key];
        }

        template<class Key>
        const item* u_get(const Key& key) const {
            auto itr = RealType::_find(cnt, key);
            return itr != cnt.end() ? &(itr->second) : nullptr;
        }

        template<class Key>
        item* u_get(const Key& key) {
            return const_cast<item*>( const_cast<const basic_map_collection*>(this)->u_get(key) );
        }

        template<class Key>
        const_iterator u_find_iterator(const Key& k) const { return RealType::_find(cnt, k); }

        template<class Key>
        bool erase(const Key& key) {
            object_lock g(this);
            return u_erase(key);
        }

        template<class Key>
        bool u_erase(const Key& key) {
            typename container_type::iterator itr = RealType::_find(cnt, key);
            return itr != cnt.end() ? (cnt.erase(itr), true) : false;
        }

        void u_clear() override {
            cnt.clear();
        }

        template<class T, class Key> item* u_set(const Key& key, T&& value) {
            return &(cnt[key] = std::forward<T>(value));
        }

        template<class T, class Key> void set(const Key& key, T&& value) {
            object_lock g(this);
            u_set(key, std::forward<T>(value));
        }

        SInt32 u_count() const override {
            return cnt.size();
        }

        template<class Key>
        item& operator [] (const Key& key) {
            return const_cast<item&>(const_cast<const basic_map_collection&>(*this)[key]);
        }

        template<class Key>
        const item& operator [] (const Key& key) const {
            auto itm = u_get(key);
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


    class map : public basic_map_collection< map, std::map<std::string, item, map_case_insensitive_comp > >
    {
    public:
        enum  {
            TypeId = CollectionType::Map,
        };

        //////////////////////////////////////////////////////////////////////////

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };

    class form_map : public basic_map_collection< form_map, std::map<form_ref, item, form_ref::stable_less_comparer> >
    {
    private:
        using base = basic_map_collection< form_map, std::map<form_ref, item, form_ref::stable_less_comparer> >;

    public:

        // form_ref_lightweight support

        using base::_find;
        using base::u_get_or_create;

        template<class ContainerType>
        static util::choose_iterator<ContainerType> _find(ContainerType& c, const form_ref_lightweight& k) {
            auto comparer = c.key_comp();
            auto itr = std::lower_bound(c.begin(), c.end(), k,
                [comparer](const form_map::value_type& pair, const form_ref_lightweight& k) {
                    return comparer(pair.first, k);
                }
            );
            return itr != c.end() && forms::form_ref::stable_equal_comparer{}(itr->first, k) ? itr : c.end();
        }

        item& u_get_or_create(const form_ref_lightweight& key) {
            return cnt[key.to_form_ref()];
        }

    public:
        enum  {
            TypeId = CollectionType::FormMap,
        };

        void u_onLoaded() override;

        //////////////////////////////////////////////////////////////////////////

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive>
        void load(Archive & ar, const unsigned int version);
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const;
    };

    class integer_map : public basic_map_collection < integer_map, std::map<int32_t, item> >
    {
    public:
        enum  {
            TypeId = CollectionType::IntegerMap,
        };

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);
    };
}
