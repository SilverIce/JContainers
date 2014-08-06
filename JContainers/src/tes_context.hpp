
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
                result = map::object(*this);
                setDataBase(result);
            }

            _lazyDBLock.unlock();
        }

        return result->as<map>();
    }
}
