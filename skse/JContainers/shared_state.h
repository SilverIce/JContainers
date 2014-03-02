#pragma once

#include "collections.h"
#include "autorelease_queue.h"

namespace collections {

    class shared_state {
        bshared_mutex _mutex;
        HandleT _databaseId;

        shared_state()
            : registry(_mutex)
            , aqueue(_mutex)
            , _databaseId(0)
        {
            setupForFirstTime();
        }

        void postLoadMaintenance();

    public:
        collection_registry registry;
        autorelease_queue aqueue;

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

        void loadAll(const std::string & data);

        void setupForFirstTime() {
            setDataBase(map::object());
        }

        std::string saveToArray();
    };

}

