#pragma once

#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

//#include <sstream>
//#include <memory>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

#include <fstream>
#include <sstream>

namespace collections {
    // static unsigned long create(StaticFunctionTag *) {

    class shared_state {
        bshared_mutex _mutex;

        shared_state()
            : registry(_mutex)
            , aqueue(_mutex)
        {
        }

    public:
        collection_registry registry;
        autorelease_queue aqueue;

        static shared_state& instance() {
            static shared_state st;
            return st;
        }

        void loadAll(const vector<char> &data) {

            _DMESSAGE("%u bytes loaded", data.size());

            typedef boost::iostreams::basic_array_source<char> Device;
            boost::iostreams::stream_buffer<Device> buffer(data.data(), data.size());
            boost::archive::binary_iarchive archive(buffer);

            write_lock g(_mutex);

            registry._clear();
            aqueue._clear();

            archive >> registry;
            archive >> aqueue;
        }

        vector<char> saveToArray() {
            vector<char> buffer;
            boost::iostreams::back_insert_device<decltype(buffer) > device(buffer);
            boost::iostreams::stream<decltype(device)> stream(device);

            // std::ofstream fstr("abjkbjklXCBjk");

            write_lock g(_mutex);

            boost::archive::binary_oarchive arch(stream);
            arch << registry;
            arch << aqueue;

            _DMESSAGE("%u bytes saved", buffer.size());

            return buffer;
        }
    };

    autorelease_queue& autorelease_queue::instance() {
        return shared_state::instance().aqueue;
    }

    collection_registry& collection_registry::instance() {
        return shared_state::instance().registry;
    }
}

