#pragma once

#include "object_context.h"
#include "collections.h"
#include "tes_error_code.h"
#include "spinlock.h"

namespace collections
{
    class map;

    class tes_context : public object_context, public shared_state_delegate
    {
        std::atomic<Handle> _databaseId;
        std::atomic_uint_fast16_t _lastError;
        spinlock _lazyDBLock;

    public:

        tes_context()
            : _databaseId(HandleNull)
            , _lastError(0)
        {
            object_context::delegate = this;
        }

        ~tes_context() {
        }

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

        Handle databaseId() const {
            return _databaseId;
        }

        map* database();

        map::ref database_ref() {
            return database();
        }

        void setDataBase(object_base *db) {
            object_base * prev = getObject(_databaseId);

            if (prev == db) {
                return;
            }

            if (db) {
                db->retain();
            }

            if (prev) {
                prev->release();
            }

            _databaseId = db ? db->uid() : HandleNull;
        }

        void u_loadAdditional(boost::archive::binary_iarchive & arch) override;
        void u_saveAdditional(boost::archive::binary_oarchive & arch) override;
        void u_cleanup() override;

        size_t collect_garbage() {
            return object_context::collect_garbage({ *database() });
        }
    };

}
