
namespace collections
{
    shared_state::shared_state()
        : registry(nullptr)
        , aqueue(nullptr)
    {
        registry = new object_registry();
        aqueue = new autorelease_queue(*registry);
    }

    shared_state::~shared_state() {

        delete registry;
        delete aqueue;
    }
    
    void shared_state::u_clearState() {

        /*  Not good, but working solution.

        issue: deadlock during loading savegame - e.g. cleaning current state.

        due to: collection release -> dealloc -> collection_registry::removeObject
        introduces deadlock ( registry locked during context cleanup )

        solution: nullify object cross-references, then delete objects

        all we need is just free the memory but this will require track allocated collection & stl memory blocks
        */

        aqueue->u_nullify();

        for (auto& pair : registry->u_container()) {
            pair.second->u_nullifyObjects();
        }
        for (auto& pair : registry->u_container()) {
            delete pair.second;
        }

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

    size_t shared_state::aqueueSize() {
        return aqueue->count();
    }

    void shared_state::loadAll(const std::string & data, int version) {

        std::istringstream stream(data);
        boost::archive::binary_iarchive archive(stream);

        aqueue->stop();
        {
            // i have assumed that Skyrim devs are not idiots to run scripts in process of save game loading
            //write_lock g(_mutex);

            u_clearState();

            if (!data.empty()) {

                if (kJSerializationCurrentVersion < version) {
                     _FATALERROR("plugin can not be compatible with future save version %u. plugin save vesrion is %u", version, kJSerializationCurrentVersion);
                     assert(false);
                }

                try {
                    archive >> *registry;
                    archive >> *aqueue;

                    if (delegate) {
                        delegate->u_loadAdditional(archive);
                    }
                }
                catch (const std::exception& exc) {
                    _FATALERROR("caught exception (%s) during archive load - '%s'. forcing application to crash",
                        typeid(exc).name(), exc.what());
                    //u_clearState();

                    // force whole app to crash
                    assert(false);
                }
                catch (...) {
                    _FATALERROR("caught unknown (non std::*) exception. forcing application to crash");
                    // force whole app to crash
                    assert(false);
                }

            }

            u_applyUpdates(version);
            // deadlock possible
            u_postLoadMaintenance(version);

            _DMESSAGE("%u objects total", registry->u_container().size());
            _DMESSAGE("%u objects in aqueue", aqueue->u_count());
        }
        aqueue->start();
    }

    std::string shared_state::saveToArray() {
        std::ostringstream stream;
        boost::archive::binary_oarchive arch(stream);

        aqueue->stop();
        {
            // i have assumed that Skyrim devs are not idiots to run scripts in process of saving
            // but didn't dare to disable all that locks

            // do not lock as test shows that skyrim waits for completion of any native function running.
            // this _probably_ means that there is no any thread inside my code during save
            //read_lock g(_mutex);
            //all_objects_lock l(*registry);

            arch << *registry;
            arch << *aqueue;

            if (delegate) {
                delegate->u_saveAdditional(arch);
            }
        }
        aqueue->start();

        std::string data(stream.str());

        return data;
    }

    //////////////////////////////////////////////////////////////////////////

    void shared_state::u_applyUpdates(int saveVersion) {

    }

    void shared_state::u_postLoadMaintenance(int saveVersion)
    {
        auto cntCopy = registry->u_container();
        static_assert( std::is_reference<decltype(cntCopy)>::value == false , "");

        for (auto& pair : cntCopy) {
            pair.second->set_context(*this);
            pair.second->u_onLoaded();
        }
    }

}
