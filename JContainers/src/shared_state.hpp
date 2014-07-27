namespace collections
{
    template<class T, class D>
    inline std::unique_ptr<T, D> make_unique_ptr(T* data, D destr) {
        return std::unique_ptr<T, D>(data, destr);
    }

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

        purpose: free allocated memory
        problem: regular delete call won't help as delete call will access memory of possible deleted object

        solution: isolate objects by nullifying cross-references, then delete objects

        all we need is just free all allocated memory but this will require track stl memory blocks
        */

        aqueue->u_nullify();

        for (auto& obj : registry->u_container()) {
            obj->u_nullifyObjects();
        }
        for (auto& obj : registry->u_container()) {
            delete obj;
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
            u_clearState();
        }
        
        aqueue->start();
    }

    std::vector<object_stack_ref> shared_state::filter_objects(std::function<bool(object_base& obj)> predicate) const {
        return registry->filter_objects(predicate);
    }

    object_base * shared_state::getObject(Handle hdl) {
        return registry->getObject(hdl);
    }

    object_stack_ref shared_state::getObjectRef(Handle hdl) {
        return registry->getObjectRef(hdl);
    }

    object_base * shared_state::u_getObject(Handle hdl) {
        return registry->u_getObject(hdl);
    }

    size_t shared_state::aqueueSize() {
        return aqueue->count();
    }

    void shared_state::read_from_string(const std::string & data, const uint32_t version) {
        namespace io = boost::iostreams;
        io::stream<io::array_source> stream( io::array_source(data.c_str(), data.size()) );
        read_from_stream(stream, version);
    }

    struct header {

        uint32_t updateVersion;

        static header imitate_old_header() {
            return{ kJSerializationNoHeaderVersion };
        }

        static header make() {
            return{ kJSerializationCurrentVersion };
        }

        static header read_from_stream(std::istream & stream) {

            uint32_t hdrSize = 0;
            stream >> hdrSize;
            std::string hdrString(hdrSize, '\0');
            stream.read((char*)hdrString.c_str(), hdrSize);

            auto js = make_unique_ptr(json_loads(hdrString.c_str(), 0, nullptr), &json_decref);

            return{ json_integer_value(json_object_get(js.get(), "commonVersion")) };
        }

        static auto write_to_json() -> decltype(make_unique_ptr((json_t *)nullptr, &json_decref)) {
            auto header = make_unique_ptr(json_object(), &json_decref);

            json_object_set(header.get(), "commonVersion", json_integer(kJSerializationCurrentVersion));

            return header;
        }

        static void write_to_stream(std::ostream & stream) {
            auto header = write_to_json();
            auto data = make_unique_ptr(json_dumps(header.get(), 0), free);

            uint32_t hdrSize = strlen(data.get());
            stream << (uint32_t)hdrSize;
            stream.write(data.get(), hdrSize);
        }
    };

    void shared_state::read_from_stream(std::istream & stream, const uint32_t version) {

        stream.flags(stream.flags() | std::ios::binary);

        aqueue->stop();
        {
            // i have assumed that Skyrim devs are not idiots to run scripts in process of save game loading
            //write_lock g(_mutex);

            u_clearState();

            auto hdr = header::make();

            bool isFromFuture = kJSerializationCurrentVersion < version;

            if (isFromFuture) {
                _FATALERROR("plugin can not be compatible with future save version %u. plugin save vesrion is %u", version, kJSerializationCurrentVersion);
                jc_assert(false);
            }

            if (stream.peek() != std::istream::traits_type::eof() && !isFromFuture) {

                if (version <= kJSerializationNoHeaderVersion) {
                    hdr = header::imitate_old_header();
                }
                else {
                    hdr = header::read_from_stream(stream);
                }

                boost::archive::binary_iarchive archive(stream);

                try {
                    archive >> *registry;
                    archive >> *aqueue;

                    if (delegate) {
                        delegate->u_loadAdditional(archive);
                    }
                }
                catch (const std::exception& exc) {
                    _FATALERROR("caught exception (%s) during archive load - '%s'",
                        typeid(exc).name(), exc.what());
                    u_clearState();

                    // force whole app to crash
                    jc_assert(false);
                }
                catch (...) {
                    _FATALERROR("caught unknown (non std::*) exception");
                    u_clearState();

                    // force whole app to crash
                    jc_assert(false);
                }
            }

            u_applyUpdates(hdr.updateVersion);
            u_postLoadMaintenance(hdr.updateVersion);

            _DMESSAGE("%lu objects total", registry->u_container().size());
            _DMESSAGE("%lu objects in aqueue", aqueue->u_count());
        }
        aqueue->start();
    }

    std::string shared_state::write_to_string() {
        std::ostringstream stream;
        write_to_stream(stream);
        return stream.str();
    }

    void shared_state::write_to_stream(std::ostream& stream) {

        stream.flags(stream.flags() | std::ios::binary);

        header::write_to_stream(stream);

        aqueue->stop();
        {
            boost::archive::binary_oarchive arch(stream);
            arch << *registry;
            arch << *aqueue;

            if (delegate) {
                delegate->u_saveAdditional(arch);
            }
            _DMESSAGE("%lu objects total", registry->u_container().size());
            _DMESSAGE("%lu objects in aqueue", aqueue->u_count());
        }
        aqueue->start();
    }

    //////////////////////////////////////////////////////////////////////////

    void shared_state::u_applyUpdates(const uint32_t saveVersion) {

    }

    void shared_state::u_postLoadMaintenance(const uint32_t saveVersion)
    {
        for (auto& obj : registry->u_container()) {
            obj->set_context(*this);
        }

        for (auto& obj : registry->u_container()) {
            obj->u_onLoaded();
        }
    }

}
