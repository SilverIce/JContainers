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

    void shared_state::loadAll(const vector<char> &data) {

        _DMESSAGE("%u bytes loaded", data.size());

        typedef boost::iostreams::basic_array_source<char> Device;
        boost::iostreams::stream_buffer<Device> buffer(data.data(), data.size());
        boost::archive::binary_iarchive archive(buffer);

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

    vector<char> shared_state::saveToArray() {
        vector<char> buffer;
        boost::iostreams::back_insert_device<decltype(buffer) > device(buffer);
        boost::iostreams::stream<decltype(device)> stream(device);
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

        _DMESSAGE("%u bytes saved", buffer.size());

        return buffer;
    }
}
