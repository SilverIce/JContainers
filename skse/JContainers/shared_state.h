#pragma once

#include <atomic>
#include "collections.h"
#include "autorelease_queue.h"
#include "tes_error_code.h"

namespace collections {

    class shared_state {
        bshared_mutex _mutex;
        HandleT _databaseId;
        std::atomic_uint_fast16_t _lastError;

        shared_state()
            : registry(_mutex)
            , aqueue(_mutex)
            , _databaseId(0)
            , _lastError(0)
        {
            clearState();
        }

        void postLoadMaintenance();

        //static shared_state _sharedInstance;

        template<class T>
        void performRead(T& readFunc) {
             read_lock r(_mutex);
             readFunc();
        }

    public:
        collection_registry registry;
        autorelease_queue aqueue;

        void setLastError(JErrorCode code) {
            _lastError = code;
        }

        JErrorCode lastError() {
            uint_fast16_t code = _lastError.exchange(0);
            return (JErrorCode)code;
        }

        static shared_state& instance() {
            static shared_state st;
            return st;
        }

        HandleT databaseId() {
            read_lock r(_mutex);
            return _databaseId;
        }

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

