//BOOST_CLASS_EXPORT_GUID(collections::shared_state, "kJObjectContext");

namespace collections
{
    shared_state::shared_state()
        : registry(* new collection_registry(_mutex) )
        , aqueue(* new autorelease_queue(registry, _mutex) )
    {

    }

    shared_state::~shared_state() {
        delete &registry;
        delete &aqueue;
    }

    object_base * shared_state::getObject(HandleT hdl) {
        return registry.getObject(hdl);
    }

    object_base * shared_state::u_getObject(HandleT hdl) {
        return registry.u_getObject(hdl);
    }

    void shared_state::loadAll(const std::string & data, int version) {

        _DMESSAGE("%u bytes loaded", data.size());

        std::istringstream stream(data);
        boost::archive::binary_iarchive archive(stream);

        aqueue.setPaused(true);
        {
            // i have assumed that Skyrim devs are not idiots to run scripts in process of save game loading
            //write_lock g(_mutex);

            u_clearState();

            if (!data.empty() && version == kJSerializationDataVersion) {
                archive >> registry;
                archive >> aqueue;

                if (delegate) {
                    delegate->u_loadAdditional(archive);
                }
            }

            postLoadMaintenance();
        }
        aqueue.setPaused(false);
    }

    void shared_state::postLoadMaintenance()
    {
        auto cntCopy = registry.u_container();
        static_assert( std::is_reference<decltype(cntCopy)>::value == false , "");

        for (auto& pair : cntCopy) {
            pair.second->_context = this;
            pair.second->u_onLoaded();
        }
    }

    void shared_state::u_clearState() {
        registry.u_clear();
        aqueue.u_clear();

        if (delegate) {
            delegate->u_cleanup();
        }
    }

    void shared_state::clearState() {
        write_lock w(_mutex);
        u_clearState();
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

            if (delegate) {
                delegate->u_saveAdditional(arch);
            }

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
