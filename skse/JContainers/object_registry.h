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
        bshared_mutex& _mutex;

        object_registry(const object_registry& );
        object_registry& operator = (const object_registry& );

    public:

        explicit object_registry(bshared_mutex &mt)
            : _mutex(mt)
        {
        }

        Handle registerObject(object_base *collection);

        void removeObject(HandleT hdl) {
            if (!hdl) {
                return;
            }

            object_registry& me = *this;
            write_lock g(me._mutex);
            auto itr = me._map.find(hdl);
            assert(itr != me._map.end());
            me._map.erase(itr);
            me._idGen.reuseId(hdl);
        }

        object_base *getObject(HandleT hdl) {
            if (!hdl) {
                return nullptr;
            }
            
            read_lock g(_mutex);
            return u_getObject(hdl);
        }

        object_base *u_getObject(HandleT hdl) {
            if (!hdl) {
                return nullptr;
            }

            object_registry& me = *this;
            auto itr = me._map.find(hdl);
            if (itr != me._map.end())
                return itr->second;

            return nullptr;
        }

        template<class T>
        T *getObjectOfType(HandleT hdl) {
            auto obj = getObject(hdl);
            return (obj && obj->_type == T::TypeId) ? static_cast<T*>(obj) : nullptr;
        }

        template<class T>
        T *u_getObjectOfType(HandleT hdl) {
            auto obj = u_getObject(hdl);
            return (obj && obj->_type == T::TypeId) ? static_cast<T*>(obj) : nullptr;
        }

        static object_registry& instance();

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
        object_registry& me = *this;
        write_lock g(me._mutex);
        auto newId = me._idGen.newId();
        assert(me._map.find(newId) == me._map.end());
        me._map[newId] = collection;
        return (Handle)newId;
    }

    void object_registry::u_clear() {
        /*  Not good, but working solution.

            issue: deadlock during loading savegame - e.g. cleaning current state.

            due to: collection release -> dealloc -> collection_registry::removeObject
                 introduces deadlock ( registry locked during _clear call)

            solution: +1 refCount to all objects & clear & delete them

            all we need is just free the memory but this will require track allocated collection & stl memory blocks 
        */
        for (auto& pair : _map) {
            pair.second->u_retain(); // to guarant that removeObject will not be called during clear method call
        }
        for (auto& pair : _map) {
            pair.second->u_clear(); // to force ~Item() calls while all collections alive (~Item() may release collection)
        }
        for (auto& pair : _map) {
            delete pair.second;
        }
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
