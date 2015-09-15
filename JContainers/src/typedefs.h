#pragma once

#include <assert.h>
#include <type_traits>

#   define STR(...)     __STR(__VA_ARGS__)
#   define __STR(...)   #__VA_ARGS__

#   define ARGS(...)    __VA_ARGS__

extern void JC_log(const char * fmt, ...);
extern void JC_log(const char* fmt, va_list& args);

#   ifdef NO_JC_DEBUG
#       define jc_assert(expr)
#       define jc_debug(message, ...)
#       define jc_assert_msg(expr, fmt, ...)
#   else
#       define jc_assert(expr)              do { if (!(expr)) { __debugbreak(); } } while(0)
#       define jc_debug(message, ...)       do { printf(message"\n", __VA_ARGS__); } while(0)

#       define jc_assert_msg(expr, fmt, ...)    \
            do { \
                if (!(expr)) { \
                    JC_log(fmt, __VA_ARGS__); \
                    assert(false && fmt); \
                } \
            } while(0)

#   endif

__declspec(noreturn) inline void noreturn_func() {}

template<typename E>
inline auto to_integral(E e) -> typename std::underlying_type<E>::type {
    return static_cast<typename std::underlying_type<E>::type>(e);
}
