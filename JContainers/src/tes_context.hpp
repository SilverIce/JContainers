
namespace collections {

    void tes_context::u_loadAdditional(boost::archive::binary_iarchive & arch) {
        arch >> _databaseId;
    }

    void tes_context::u_saveAdditional(boost::archive::binary_oarchive & arch) {
        arch << _databaseId;
    }

    void tes_context::u_cleanup() {
        _databaseId = HandleNull;
        _lastError = 0;
    }

    map* tes_context::database() {

        auto getDB = [&]() {
            object_base * result = nullptr;
            performRead([&]() {
                result = u_getObject(_databaseId);
            });

            return result;
        };

        object_base * result = getDB();

        if (!result) {
            _lazyDBLock.lock();

            result = getDB();
            if (!result) {
                result = map::object(*this);
                setDataBase(result);
            }

            _lazyDBLock.unlock();
        }

        return result->as<map>();
    }
}
