
namespace collections {

    void tes_context::u_loadAdditional(boost::archive::binary_iarchive & arch) {
        Handle id = HandleNull;
        arch >> id;
        _databaseId.store(id, std::memory_order_relaxed);
    }

    void tes_context::u_saveAdditional(boost::archive::binary_oarchive & arch) {
        auto id = _databaseId.load(std::memory_order_relaxed);
        arch << id;
    }

    void tes_context::u_cleanup() {
        _databaseId = HandleNull;
        _lastError = 0;
    }

    map* tes_context::database() {

        object_base * result = getObject(_databaseId);

        if (!result) {
            _lazyDBLock.lock();

            result = getObject(_databaseId);
            if (!result) {
                result = &map::object(*this);
                setDataBase(result);
            }

            _lazyDBLock.unlock();
        }

        return result->as<map>();
    }

    void tes_context::setDataBase(object_base *db) {
        object_base * prev = getObject(_databaseId);

        if (prev == db) {
            return;
        }

        if (db) {
            db->retain();
            db->tes_retain(); // emulates a user-who-needs @db, this will prevent @db from being garbage collected
        }

        if (prev) {
            prev->release();
            db->tes_retain();
        }

        _databaseId = db ? db->uid() : HandleNull;
    }

    void tes_context::u_applyUpdates(const serialization_version saveVersion) {
        if (saveVersion <= serialization_version::pre_gc) {
            if (auto db = database()) {
                db->tes_retain();
            }
        }
    }

}
