#include "util/util.h"
#include <fstream>

namespace collections
{
    template<class T, class D>
    inline std::unique_ptr<T, D> make_unique_ptr(T* data, D destr) {
        return std::unique_ptr<T, D>(data, destr);
    }

    object_context::object_context()
        : registry(nullptr)
        , aqueue(nullptr)
        , _root_object_id(HandleNull)
    {
        registry = new object_registry();
        aqueue = new autorelease_queue(*registry);
    }

    object_context::~object_context() {
        shutdown();

        delete registry;
        delete aqueue;
    }
    
    void object_context::u_clearState() {
        {
            spinlock::guard g(_dependent_contexts_mutex);
            for (auto& ctx : _dependent_contexts) {
                ctx->clear_state();
            }
        }

        _root_object_id = HandleNull;

        /*  Not good, but working solution.

        purpose: free allocated memory
        problem: a regular delete call won't help as the delete will access memory of possible deleted objects

        solution: isolate the objects by nullifying cross-references, then delete the objects

        actually all I need is just free all allocated memory, but this is hardly achievable

        */
        {
            aqueue->u_nullify();

            for (auto& obj : registry->u_all_objects()) {
                obj->u_nullifyObjects();
            }
            for (auto& obj : registry->u_all_objects()) {
                delete obj;
            }

            registry->u_clear();
            aqueue->u_clear();
        }
    }

    void object_context::shutdown() {
        aqueue->stop();
        u_clearState();
    }

    void object_context::clearState() {
        aqueue->stop();
        u_clearState();
        aqueue->start();
    }

    std::vector<object_stack_ref> object_context::filter_objects(std::function<bool(object_base& obj)> predicate) const {
        return registry->filter_objects(predicate);
    }

    object_base * object_context::getObject(Handle hdl) {
        return registry->getObject(hdl);
    }

    object_stack_ref object_context::getObjectRef(Handle hdl) {
        return registry->getObjectRef(hdl);
    }

    object_base * object_context::u_getObject(Handle hdl) {
        return registry->u_getObject(hdl);
    }

    size_t object_context::aqueueSize() {
        return aqueue->count();
    }

    size_t object_context::collect_garbage() {
        aqueue->stop();
        auto res = garbage_collector::u_collect(*registry, *aqueue);
        aqueue->start();
        return res.garbage_total;
    }

    //////////////////////////////////////////////////////////////////////////

    void object_context::read_from_string(const std::string & data) {
        namespace io = boost::iostreams;
        io::stream<io::array_source> stream( io::array_source(data.c_str(), data.size()) );
        read_from_stream(stream);
    }

    struct header {

        serialization_version updateVersion;

        static header imitate_old_header() {
            return{ serialization_version::no_header };
        }

        static header make() {
            return{ serialization_version::current };
        }

        static const char *common_version_key() { return "commonVersion"; }

        static header read_from_stream(std::istream & stream) {

            uint32_t hdrSize = 0;
            stream >> hdrSize;

            std::vector<char> buffer(hdrSize);
            stream.read(&buffer.front(), buffer.size());

            auto js = make_unique_ptr(json_loadb(&buffer.front(), buffer.size(), 0, nullptr), &json_decref);
            if (!js) { // parsing failed
                return imitate_old_header();
            }

            return{ (serialization_version)json_integer_value(json_object_get(js.get(), common_version_key())) };
        }

        static auto write_to_json() -> decltype(make_unique_ptr((json_t *)nullptr, &json_decref)) {
            auto header = make_unique_ptr(json_object(), &json_decref);

            json_object_set(header.get(), common_version_key(), json_integer((json_int_t)serialization_version::current));

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

    void object_context::read_from_stream(std::istream & stream) {

        stream.flags(stream.flags() | std::ios::binary);

#       if 0
        std::ofstream file("dump", std::ios::binary | std::ios::out);
        std::copy(
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>(),
            std::ostreambuf_iterator<char>(file)
            );
        file.close();
#       endif

        aqueue->stop();
        {
            // i have assumed that Skyrim devs are not idiots to run scripts in process of save game loading
            //write_lock g(_mutex);

            u_clearState();

            if (stream.peek() != std::istream::traits_type::eof()) {

                try {

                    auto hdr = header::read_from_stream(stream);
                    bool isNotSupported = serialization_version::current < hdr.updateVersion || hdr.updateVersion <= serialization_version::no_header;

                    if (isNotSupported) {
                        std::ostringstream error;
                        error << "Unable to load serialized data of version " << (int)hdr.updateVersion
                            << ". Current serialization version is " << (int)serialization_version::current;
                        throw std::logic_error(error.str());
                    }

                    {
                        boost::archive::binary_iarchive archive(stream);
                        archive >> *registry;
                        archive >> *aqueue;
                        boost::serialization::load_atomic(archive, _root_object_id);
                    }

                    {
                        for (auto& obj : registry->u_all_objects()) {
                            obj->set_context(*this);
                        }
                        for (auto& obj : registry->u_all_objects()) {
                            obj->u_onLoaded();
                        }
                    }

                    u_applyUpdates(hdr.updateVersion);
                    u_postLoadMaintenance(hdr.updateVersion);
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

            u_print_stats();
        }
        aqueue->start();
    }

    std::string object_context::write_to_string() {
        std::ostringstream stream;
        write_to_stream(stream);
        return stream.str();
    }


    template<class T>
    std::string serialize_g(const char* str) {
        std::stringstream stream;
        boost::archive::binary_oarchive arch(stream);
        arch << T(str);
        return stream.str();
    }

    TEST(idiioti,_)
    {
        
        auto toto = "kill me!";
        auto s1 = serialize_g<std::string>(toto);
        auto s2 = serialize_g<util::istring>(toto);
    }

    void object_context::write_to_stream(std::ostream& stream) {

        stream.flags(stream.flags() | std::ios::binary);

        header::write_to_stream(stream);

        aqueue->stop();
        {
            boost::archive::binary_oarchive arch(stream);
            arch << *registry;
            arch << *aqueue;
            boost::serialization::save_atomic(arch, _root_object_id);

            u_print_stats();
        }
        aqueue->start();
    }

    void object_context::u_print_stats() const {
        _DMESSAGE("%lu objects total", registry->u_all_objects().size());
        _DMESSAGE("%lu public objects", registry->u_public_object_count());
        _DMESSAGE("%lu objects in aqueue", aqueue->u_count());
    }

    //////////////////////////////////////////////////////////////////////////

    void object_context::u_applyUpdates(const serialization_version saveVersion) {
        if (saveVersion <= serialization_version::pre_gc) {
            if (auto db = root()) {
                db->tes_retain();
            }
        }
    }

    void object_context::u_postLoadMaintenance(const serialization_version saveVersion)
    {
        util::do_with_timing("Garbage collection", [&]() {
            auto res = garbage_collector::u_collect(*registry, *aqueue);
            _DMESSAGE("%u garbage objects collected. %u objects are parts of cyclic graphs", res.garbage_total, res.part_of_graphs);
        });
    }

    //////////////////////////////////////////////////////////////////////////

    object_base* object_context::root() {
        return getObject(_root_object_id);
    }

    void object_context::set_root(object_base *db) {
        object_base * prev = getObject(_root_object_id);

        if (prev == db) {
            return;
        }

        if (db) {
            //db->retain();
            db->tes_retain(); // emulates a user-who-needs @root, this will prevent @db from being garbage collected
        }

        if (prev) {
            //prev->release();
            prev->tes_release();
        }

        _root_object_id = db ? db->uid() : HandleNull;
    }

    //////////////////////////////////////////////////////////////////////////

    void object_context::add_dependent_context(dependent_context& ctx) {
        spinlock::guard g(_dependent_contexts_mutex);
        if (std::find(_dependent_contexts.begin(), _dependent_contexts.end(), &ctx) == _dependent_contexts.end()) {
            _dependent_contexts.push_back(&ctx);
        }
    }

    void object_context::remove_dependent_context(dependent_context& ctx) {
        spinlock::guard g(_dependent_contexts_mutex);
        _dependent_contexts.erase(std::remove(_dependent_contexts.begin(), _dependent_contexts.end(), &ctx), _dependent_contexts.end());
    }


}
