#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>

#include "jansson.h"

BOOST_CLASS_VERSION(collections::tes_context, 1);

namespace collections {

    template<class T, class D>
    inline std::unique_ptr<T, D> make_unique_ptr(T* data, D destr) {
        return std::unique_ptr<T, D>(data, destr);
    }

    struct header {

        serialization_version commonVersion;

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

    void tes_context::read_from_stream(std::istream & stream) {

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

        activity_stopper stopper{ *this };
        {
            // i have assumed that Skyrim devs are not idiots to run scripts in process of save game loading
            //write_lock g(_mutex);

            u_clearState();

            if (stream.peek() != std::istream::traits_type::eof()) {

                try {

                    auto hdr = header::read_from_stream(stream);
                    bool isNotSupported = serialization_version::current < hdr.commonVersion || hdr.commonVersion <= serialization_version::no_header;

                    if (isNotSupported) {
                        std::ostringstream error;
                        error << "Unable to load serialized data of version " << (int)hdr.commonVersion
                            << ". Current serialization version is " << (int)serialization_version::current;
                        throw std::logic_error(error.str());
                    }

                    {
                        boost::archive::binary_iarchive archive{ stream };

                        if (hdr.commonVersion <= serialization_version::pre_dyn_form_watcher) {
                            load_data_in_old_way(archive);
                        } else {
                            archive >> *this;
                        }
                    }

                    u_postLoadInitializations();
                    u_applyUpdates(hdr.commonVersion);
                    u_postLoadMaintenance(hdr.commonVersion);
                }
                catch (const std::exception& exc) {
                    _FATALERROR("caught exception (%s) during archive load - '%s'",
                        typeid(exc).name(), exc.what());
                    u_clearState();

                    // force whole app to crash
                   // jc_assert(false);
                }
                catch (...) {
                    _FATALERROR("caught unknown (non std::*) exception");
                    u_clearState();

                    // force whole app to crash
                    //jc_assert(false);
                }
            }

            u_print_stats();
            JC_log("%lu non persistent forms being watched", form_watcher->u_forms_count());
        }
    }

    void tes_context::write_to_stream(std::ostream& stream) {

        stream.flags(stream.flags() | std::ios::binary);

        header::write_to_stream(stream);

        activity_stopper s{ *this };
        {
            boost::archive::binary_oarchive arch{ stream };
            arch << *this;
            u_print_stats();
        }
    }

    void tes_context::read_from_string(const std::string & data) {
        namespace io = boost::iostreams;
        io::stream<io::array_source> stream(io::array_source(data.c_str(), data.size()));
        read_from_stream(stream);
    }

    std::string tes_context::write_to_string() {
        std::ostringstream stream;
        write_to_stream(stream);
        return stream.str();
    }

    template<class Archive> void tes_context::load_data_in_old_way(Archive& ar) {
        base::load_data_in_old_way(ar);
        boost::serialization::load_atomic(ar, _root_object_id);
    }

    template<class Archive> void tes_context::load(Archive & ar, unsigned int version) {
        ar >> static_cast<base&>(*this);
        boost::serialization::load_atomic(ar, _root_object_id);

        if (version >= 1) {
            form_watching::dyn_form_watcher* watcher = nullptr;
            ar >> watcher;

            form_watcher.reset(watcher);
        }
    }

    template<class Archive> void tes_context::save(Archive & ar, unsigned int version) const {
        form_watching::dyn_form_watcher* watcher = form_watcher.get();

        ar << static_cast<const base&>(*this);
        boost::serialization::save_atomic(ar, _root_object_id);
        ar << watcher;
    }

    ///////////////////////////

    void tes_context::u_applyUpdates(const serialization_version saveVersion) {
        if (saveVersion <= serialization_version::pre_gc) {
            if (auto db = u_getObject(_root_object_id.load(std::memory_order_relaxed))) {
                db->tes_retain();
            }
        }
    }

    ////////////////////////

    void tes_context::shutdown() {
        stop_activity();
        u_clearState();
    }

    void tes_context::clearState() {
        activity_stopper s{ *this };
        u_clearState();
    }

    ////////////////////////////

    void tes_context::set_root(object_base *db) {
        object_base * prev = getObject(_root_object_id.load(std::memory_order_relaxed));

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

        _root_object_id.store(db ? db->uid() : Handle::Null, std::memory_order_relaxed);
    }

    map& tes_context::root()
    {
        map * result = _cached_root.load(std::memory_order_acquire);
        if (!result) {

            spinlock::guard g(_lazyRootInitLock);

            result = _cached_root.load(std::memory_order_relaxed);
            if (!result) {
                result = base::getObjectOfType<map>(_root_object_id.load(std::memory_order_relaxed));
                if (!result) {
                    result = &map::object(*this);
                    set_root(result);
                }

                _cached_root.store(result, std::memory_order_release);
            }
        }

        return *result;
    }

    //////////////////////////////////////////////////////////////////////////

}
