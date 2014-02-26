#include <type_traits>

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

            registry.u_clear();
            aqueue.u_clear();
            _databaseId = 0;
        }

        setupForFirstTime();
    }

    void shared_state::loadAll(const std::string &data) {

        _DMESSAGE("%u bytes loaded", data.size());

        std::istringstream stream(data);
        boost::archive::binary_iarchive archive(stream);

        aqueue.setPaused(true);
        {
            // i have assumed that Skyrim devs are not idiots to run scripts in process of save game loading
            //write_lock g(_mutex);

            registry.u_clear();
            aqueue.u_clear();
            _databaseId = 0;

            archive >> registry;
            archive >> aqueue;
            archive >> _databaseId;

            // post serialization

            auto cntCopy = registry.u_container();
            static_assert( std::is_reference<decltype(cntCopy)>::value == false , "");

            for (auto& pair : cntCopy) {
                form_map *fmap = pair.second->as<form_map>();
                if (fmap) {
                    fmap->u_updateKeys();
                }
            }

        }
        aqueue.setPaused(false);
    }

    std::string shared_state::saveToArray() {
        std::ostringstream stream;
        boost::archive::binary_oarchive arch(stream);

        aqueue.setPaused(true);
        {
            // i have assumed that Skyrim devs are not idiots to run scripts in process of saving
            // but didn't dare to disable all that locks
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
        aqueue.setPaused(false);

        std::string data(stream.str());

        _DMESSAGE("%u bytes saved", data.size());

        return data;
    }
}
