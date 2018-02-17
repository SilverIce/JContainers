#pragma once

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/thread/lock_guard.hpp>

namespace collections {

    typedef boost::shared_mutex bshared_mutex;
    typedef boost::lock_guard<bshared_mutex> write_lock;
    typedef boost::shared_lock_guard<bshared_mutex> read_lock;

}