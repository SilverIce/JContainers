#pragma once

#include <atomic>
#include "object_base.h"

namespace boost {
namespace archive {
    class binary_iarchive;
    class binary_oarchive;
}
}

namespace collections {

    class object_registry;
    class autorelease_queue;

    struct shared_state_delegate
    {
        virtual void u_loadAdditional(boost::archive::binary_iarchive & arch) = 0;
        virtual void u_saveAdditional(boost::archive::binary_oarchive & arch) = 0;
        virtual void u_cleanup() = 0;
    };

    class shared_state {

        void u_applyUpdates(int saveVersion);
        void u_postLoadMaintenance(int saveVersion);

    public:

        shared_state();
        ~shared_state();

        object_registry* registry;
        autorelease_queue* aqueue;

        shared_state_delegate *delegate;

        template<class T>
        T * getObjectOfType(Handle hdl) {
            return getObject(hdl)->as<T>();
        }

        template<class T>
        object_stack_ref_template<T> getObjectRefOfType(Handle hdl) {
            return getObjectRef(hdl)->as<T>();
        }

        size_t aqueueSize();
        object_base * getObject(Handle hdl);
        object_stack_ref getObjectRef(Handle hdl);
        object_base * u_getObject(Handle hdl);

        void clearState();
        void u_clearState();

        void read_from_string(const std::string & data, int version);
        void read_from_stream(std::istream & data, int version);

        std::string write_to_string();
        void write_to_stream(std::ostream& stream);


    };

}

