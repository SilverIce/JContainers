
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <vector>
#include <atomic>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "rw_mutex.h"

#include "plugin_info.h"
#include "object_base.h"
#include "shared_state.h"

#include "id_generator.h"
#include "object_registry.h"
#include "autorelease_queue.h"


#include "object_base.hpp"
#include "shared_state.hpp"

namespace collections
{




}