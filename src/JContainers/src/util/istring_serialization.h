#pragma once

#include <boost/config.hpp>
#include <boost/serialization/level.hpp>

#include "util/istring.h"

/**
 * This pre 4 version specification needs some clarification.
 * 1. It looks actually ignored by Boost who just uses primitive type by default
 * 2. Primitive serialization cant handle pointers used by this object? For example
 *    in Debug build, basic_string is using proxy container which gets screwed.
 * Current solution: cast to std::string when serializing.
 */

BOOST_CLASS_IMPLEMENTATION(util::istring, boost::serialization::primitive_type)
