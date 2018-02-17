#pragma once

#include <boost/range/algorithm/find_if.hpp>
#include <boost/algorithm/string/find.hpp>
#include <utility>

namespace boost {

    template< typename RangeT, typename Range2T >
    inline std::pair<RangeT,RangeT> half_split(
        const RangeT& Input,
        const Range2T& Search)
    {
        RangeT substr = boost::find_first(Input, Search);
        if (!substr.empty()) {
            return std::make_pair(
                RangeT (Input.begin(), substr.begin()),
                RangeT (substr.end(), Input.end())
                );
        }

        auto end = Input.end();
        return std::make_pair(RangeT(end,end), RangeT(end,end));
    }

    template< typename RangeT, typename Predicate >
    inline std::pair<RangeT,RangeT> half_split_if(
        const RangeT& Input,
        Predicate Cond)
    {
        auto location = boost::find_if(Input, Cond);
        if (location != Input.end()) {
            return std::make_pair(
                RangeT (Input.begin(), location),
                RangeT (location + 1, Input.end())
                );
        }

        auto end = Input.end();
        return std::make_pair(RangeT(end,end), RangeT(end,end));
    }

    template< typename RangeT, typename Iter >
    inline std::pair<RangeT,RangeT> split_right_inclusive(
        const RangeT& Input,
        const Iter& location)
    {
        if (location != Input.end()) {
            return std::make_pair(
                RangeT (Input.begin(), location),
                RangeT (location, Input.end())
                );
        }

        auto end = Input.end();
        return std::make_pair(RangeT(end,end), RangeT(end,end));
    }

    template< typename RangeT, typename Iter >
    inline std::pair<RangeT,RangeT> split_left_inclusive(
        const RangeT& Input,
        const Iter& location)
    {
        if (location != Input.end()) {
            return std::make_pair(
                RangeT (Input.begin(), location + 1),
                RangeT (location + 2, Input.end())
                );
        }

        auto end = Input.end();
        return std::make_pair(RangeT(end,end), RangeT(end,end));
    }



}
