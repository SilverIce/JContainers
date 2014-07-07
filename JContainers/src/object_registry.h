namespace collections
{
    class object_registry
    {
    public:
        typedef std::map<HandleT, object_base *> registry_container;
        typedef id_generator<HandleT> id_generator_type;

    private:

        friend class shared_state;


        registry_container _map;
        id_generator<HandleT> _idGen;
        bshared_mutex _mutex;

        object_registry(const object_registry& );
        object_registry& operator = (const object_registry& );

    public:

        explicit object_registry()
            : _mutex()
        {
        }

        Handle registerObject(object_base *collection);

        void removeObject(Handle hdl) {
            jc_assert(hdl != HandleNull);

            write_lock g(_mutex);
            auto itr = _map.find(hdl);
            jc_assert(itr != _map.end());
            _map.erase(itr);
            _idGen.reuseId(hdl);
        }

        object_base *getObject(Handle hdl) {
            if (!hdl) {
                return nullptr;
            }
            
            read_lock g(_mutex);
            return u_getObject(hdl);
        }

        object_base *u_getObject(Handle hdl) {
            if (!hdl) {
                return nullptr;
            }

            auto itr = _map.find(hdl);
            if (itr != _map.end())
                return itr->second;

            return nullptr;
        }

        template<class T>
        T *getObjectOfType(HandleT hdl) {
            auto obj = getObject(hdl);
            return obj->as<T>();
        }

        template<class T>
        T *u_getObjectOfType(HandleT hdl) {
            auto obj = u_getObject(hdl);
            return obj->as<T>();
        }

        void u_clear();

        registry_container& u_container() {
            return _map;
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & _map;
            ar & _idGen;
        }
    };

    Handle object_registry::registerObject(object_base *collection)
    {
        write_lock g(_mutex);
        auto newId = _idGen.newId();
        jc_assert(_map.find(newId) == _map.end());
        _map[newId] = collection;
        return (Handle)newId;
    }

    void object_registry::u_clear() {
        _map.clear();
        _idGen.u_clear();
    }

    struct all_objects_lock
    {
        object_registry &_registry;

        explicit all_objects_lock(object_registry & registry)
            : _registry(registry)
        {
            for (auto& pair : registry.u_container()) {
                pair.second->_mutex.lock();
            }
        }

        ~all_objects_lock() {
            for (auto& pair : _registry.u_container()) {
                pair.second->_mutex.unlock();
            }
        }

    };


}

//BOOST_CLASS_EXPORT_GUID(collections::collection_registry, "kObjectRegistry");
//BOOST_CLASS_EXPORT_GUID(collections::collection_registry::id_generator_type, "kJObjectIdGenerator");
