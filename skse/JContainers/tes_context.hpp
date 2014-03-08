
namespace collections {

    void tes_context::u_loadAdditional(boost::archive::binary_iarchive & arch) {
        arch >> _databaseId;
    }

    void tes_context::u_saveAdditional(boost::archive::binary_oarchive & arch) {
        arch << _databaseId;
    }

    void tes_context::u_cleanup() {
        _databaseId = 0;
        _lastError = 0;
    }

    object_base* tes_context::database() {
        object_base * result = nullptr;
        performRead([&]() {
            result = u_getObject(_databaseId);
        });

        if (!result) {
            result = map::object(*this);
            setDataBase(result);
        }

        return result;
    }
}
