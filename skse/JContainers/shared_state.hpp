
namespace collections
{
    shared_state::shared_state()
        : registry(nullptr)
        , aqueue(nullptr)
    {
        registry = new object_registry(_mutex);
        aqueue = new autorelease_queue(*registry, _mutex);
    }

    shared_state::~shared_state() {

        delete registry;
        delete aqueue;
    }
    
    void shared_state::u_clearState() {
        registry->u_clear();
        aqueue->u_clear();
        
        if (delegate) {
            delegate->u_cleanup();
        }
    }
    
    void shared_state::clearState() {
        
        aqueue->stop();
        
        {
            write_lock w(_mutex);
            all_objects_lock l(*registry);
            u_clearState();
        }
        
        aqueue->start();
    }

    object_base * shared_state::getObject(HandleT hdl) {
        return registry->getObject(hdl);
    }

    object_base * shared_state::u_getObject(HandleT hdl) {
        return registry->u_getObject(hdl);
    }

    void shared_state::loadAll(const std::string & data, int version) {

        _DMESSAGE("%u bytes loaded", data.size());

        std::istringstream stream(data);
        boost::archive::binary_iarchive archive(stream);

        aqueue->stop();
        {
            // i have assumed that Skyrim devs are not idiots to run scripts in process of save game loading
            //write_lock g(_mutex);

            u_clearState();

            if (!data.empty() && version == kJSerializationDataVersion) {

                try {
                    archive >> *registry;
                    archive >> *aqueue;

                    if (delegate) {
                        delegate->u_loadAdditional(archive);
                    }
                }
                catch (const boost::archive::archive_exception& exc) {
                    _DMESSAGE("caught exception during archive load - '%s'. trying to recover", exc.what());

                    throw exc;

                    //u_clearState();
                }

            }

            // deadlock possible
            u_postLoadMaintenance();

            _DMESSAGE("%u objects total", registry->u_container().size());
            _DMESSAGE("%u objects in aqueue", aqueue->u_count());
        }
        aqueue->start();
    }

    void shared_state::u_postLoadMaintenance()
    {
        auto cntCopy = registry->u_container();
        static_assert( std::is_reference<decltype(cntCopy)>::value == false , "");

        for (auto& pair : cntCopy) {
            pair.second->_context = this;
            pair.second->u_onLoaded();
        }
    }


    std::string shared_state::saveToArray() {
        std::ostringstream stream;
        boost::archive::binary_oarchive arch(stream);

        aqueue->stop();
        {
            // i have assumed that Skyrim devs are not idiots to run scripts in process of saving
            // but didn't dare to disable all that locks
            read_lock g(_mutex);

            all_objects_lock l(*registry);

            arch << *registry;
            arch << *aqueue;

            if (delegate) {
                delegate->u_saveAdditional(arch);
            }
        }
        aqueue->start();

        std::string data(stream.str());

        _DMESSAGE("%u bytes saved", data.size());

        return data;
    }
}
