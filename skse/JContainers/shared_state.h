#pragma once

#include <atomic>
//#include <functional>

#include "rw_mutex.h"

#include "tes_error_code.h"
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

        void u_postLoadMaintenance();

    protected:
        bshared_mutex _mutex;
    public:

        template<class T>
        inline void performRead(T& readFunc) {
            read_lock r(_mutex);
            readFunc();
        }

        shared_state();
        ~shared_state();

        object_registry& registry;
        autorelease_queue& aqueue;

        shared_state_delegate *delegate;

        template<class T>
        T * getObjectOfType(HandleT hdl) {
            return getObject(hdl)->as<T>();
        }

        object_base * getObject(HandleT hdl);
        object_base * u_getObject(HandleT hdl);

        void clearState();
        void u_clearState();

        void loadAll(const std::string & data, int version);

        std::string saveToArray();


    };

}

