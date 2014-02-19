namespace collections {

    autorelease_queue& autorelease_queue::instance() {
        return shared_state::instance().aqueue;
    }

    collection_registry& collection_registry::instance() {
        return shared_state::instance().registry;
    }

    void shared_state::clearState() { 
        {
            write_lock g(_mutex);

            registry._clear();
            aqueue._clear();
            _databaseId = 0;
        }

        setupForFirstTime();
    }

    void shared_state::loadAll(const string &data) {

        _DMESSAGE("%u bytes loaded", data.size());

        std::istringstream stream(data);
        boost::archive::binary_iarchive archive(stream);

        {
            write_lock g(_mutex);

            registry._clear();
            aqueue._clear();
            _databaseId = 0;

            archive >> registry;
            archive >> aqueue;
            archive >> _databaseId;
        }
    }

    string shared_state::saveToArray() {
        std::ostringstream stream;
        boost::archive::binary_oarchive arch(stream);

        {
            read_lock g(_mutex);

            for (auto pair : registry._map) {
                pair.second->_mutex.lock();
            }

            arch << registry;
            arch << aqueue;
            arch << _databaseId;

            for (auto pair : registry._map) {
                pair.second->_mutex.unlock();
            }
        }

        string data(stream.str());

        _DMESSAGE("%u bytes saved", data.size());

        return data;
    }
}
