#pragma once

#include "boost/range/iterator_range_core.hpp"

namespace util {

    using cstring = boost::iterator_range<const char*>;

    inline auto make_cstring_safe(const char* cstr, size_t limit = 1024) -> cstring {
        return cstr ? boost::make_iterator_range(cstr, cstr + strnlen_s(cstr, limit)) : boost::make_iterator_range_n("", 0);
    }

}
