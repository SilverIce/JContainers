#pragma once

namespace collections {

    class shared_state {
        bshared_mutex _mutex;

        shared_state()
            : registry(_mutex)
            , aqueue(_mutex)
            , _databaseId(0)
        {
            setupForFirstTime();
        }

    public:
        collection_registry registry;
        autorelease_queue aqueue;
        HandleT _databaseId;

        static shared_state& instance() {
            static shared_state st;
            return st;
        }

        HandleT databaseId() {
            read_lock r(_mutex);
            return _databaseId;
        }

/*
        void setDataBaseId(HandleT hdl) {
            setDataBase(registry.getObject(hdl));
        }*/

        map* database() {
            return registry.getObjectOfType<map>(databaseId());
        }

        void setDataBase(object_base *db) {
            auto prev = registry.getObject(databaseId());
            if (prev == db) {
                return;
            }

            if (prev) {
                prev->release(); // may cause deadlock in removeObject
            }

            if (db) {
                db->retain();
            }

            write_lock g(_mutex);
            _databaseId = db ? db->id : 0;
        }

        void clearState();

        void loadAll(const string & data);

        void setupForFirstTime() {
            setDataBase(map::object());
        }

        string saveToArray();
    };

}

