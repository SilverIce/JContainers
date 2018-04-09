#pragma once

#include "../dep/googletest/googletest/googletest/include/gtest/gtest.h"

#define EXPECT_NOT_NIL(expr) EXPECT_NE((expr), nullptr)
#define EXPECT_NIL(expr) EXPECT_EQ((expr), nullptr)
