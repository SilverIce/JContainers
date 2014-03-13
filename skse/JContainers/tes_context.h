#pragma once

#include "shared_state.h"
#include "collections.h"
#include "tes_error_code.h"
#include "spinlock.h"

namespace collections
{

    class tes_context : public shared_state, public shared_state_delegate
    {
        HandleT _databaseId;
        std::atomic_uint_fast16_t _lastError;
        spinlock _lazyDBLock;

    public:

        tes_context()
            : _databaseId(0)
            , _lastError(0)
        {
            shared_state::delegate = this;
        }

        //~tes_context() {}

        void setLastError(JErrorCode code) {
            _lastError = code;
        }

        JErrorCode lastError() {
            uint_fast16_t code = _lastError.exchange(0);
            return (JErrorCode)code;
        }

        static tes_context& instance() {
            static tes_context st;
            return st;
        }

        HandleT databaseId() {
            read_lock r(_mutex);
            return _databaseId;
        }

        object_base* database();

        void setDataBase(object_base *db) {
            object_base * prev = nullptr;
            performRead([&]() {
                prev = u_getObject(_databaseId);
            });

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

        void u_loadAdditional(boost::archive::binary_iarchive & arch) override;
        void u_saveAdditional(boost::archive::binary_oarchive & arch) override;
        void u_cleanup() override;

    };

}
