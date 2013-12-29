#include <hash_map>
#include <assert.h>

 typedef unsigned int Handle;

template<class T>
class Registry
{

        stdext::hash_map<Handle, T *> _map;
        Handle _counter;

        Registry() {}
        Registry(const Registry& );
        Registry& operator = (const Registry& );

    public:

        static Handle registerObject(T *collection) {
            if (collection == nullptr)
                return 0;

            auto existing = getObject(collection->id);
            if (existing) {
                assert(existing == collection);
                return collection->id;
            }
            
            auto& me = instance();
            Handle id = ++me._counter;
            collection->id = id;
            me._map[id] = collection;
            return id;
        }

        static void removeObject(Handle hdl) {
            instance()._map.erase(hdl);
        }

        /*
        static T *getObject(Handle hdl) {
            collection_registry& me = instance();
            auto itr = me._map.find(hdl);
            return itr != me._map.end() ? itr->second : nullptr;
        }*/

           
        static T *getObject(Handle hdl) {
            auto& me = instance();
            auto itr = me._map.find(hdl);
            if (itr != me._map.end())
                return itr->second;

            return nullptr;
        }

        template<class T>
        static T *getObjectOfType(Handle hdl) {
                    auto obj = getObject(hdl);
                    return (obj && obj->_type == T::TypeId) ? (T*)obj : nullptr;
        }
    
        static Registry& instance() {
            static Registry _instance;
            return _instance;
        }
};
