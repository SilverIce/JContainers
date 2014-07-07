#pragma once

#include <assert.h>

#define STR(...) #__VA_ARGS__

#   ifdef _DEBUG
#       define jc_assert(expr)     if (!(expr)) { __debugbreak(); }
#   else
#       define jc_assert(expr)     assert(expr)
#   endif
