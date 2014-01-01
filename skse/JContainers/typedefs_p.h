#pragma once

#include "typedefs.h"
#include <stdexcept>

namespace Movement
{
    extern void log_write(const char* fmt, ...);
    extern void log_console(const char* str, ...);
    extern void log_write_trace();

#define CountOf(array) (sizeof(array)/sizeof(array[0]))

#ifndef static_assert
    #define CONCAT(x, y) CONCAT1 (x, y)
    #define CONCAT1(x, y) x##y
    #define static_assert(expr, msg) typedef char CONCAT(static_assert_failed_at_line_, __LINE__) [(expr) ? 1 : -1]
#endif

#define mov_assert(expr) { \
    if (!(expr)){ \
        __debugbreak(); \
        ::Movement::log_write("In "__FUNCTION__": assertion '" #expr "' failed"); \
    } }

#   ifdef ASSERTION_NOT_THROWS
#       define assert_or_throw(expr, ...) mov_assert(expr)
#       define assert_or_throw_msg(expr, message, ...) assert_or_throw(expr, )
#   else

        template<class ExceptionSource, int Reason = -1>
        class Exception : public Exception<ExceptionSource,-1> {
        public:
            explicit Exception(const char* Message) : Exception<ExceptionSource,-1>(Message) {}
        };

        template<class ExceptionSource>
        class Exception<ExceptionSource, -1> : public std::runtime_error {
        public:
            explicit Exception(const char* Message) : std::runtime_error(Message) {}
        };

#       define assert_or_throw(expr, ... /*exception type*/) \
            if (!(expr)) \
                throw __VA_ARGS__("In "__FUNCTION__": assertion '" #expr "' failed and exception of type '" #__VA_ARGS__ "' is thrown.");

#       define assert_or_throw_msg(expr, message,  ... /*exception type*/) \
            if (!(expr)) \
                throw __VA_ARGS__("In "__FUNCTION__": assertion '" #expr "' failed and exception of type '" #__VA_ARGS__ "' is thrown. " message);
#   endif

#   define ARGS(...) __VA_ARGS__

/** Use it to validate object state */
#define assert_state(expr) mov_assert(expr)

/** Use it to validate object state */
#define assert_state_msg(expr, error_msg, ...) \
    if ((expr) == false) { \
        ::Movement::log_write("In "__FUNCTION__" assertion '"#expr"' failed:\n" \
                              "   " error_msg, __VA_ARGS__); \
        __debugbreak(); \
    }

#define log_function(msg, ...)  log_write(__FUNCTION__ ": " msg, __VA_ARGS__) \

#define log_debug log_console

#   ifndef nullptr
#       ifdef __cplusplus
#           define nullptr    0
#       else
#           define nullptr    ((void *)0)
#       endif
#   endif

    template<class T, T limit>
    class counter
    {
    public:
        counter() { init();}

        void Increase()
        {
            if (m_counter == limit)
                init();
            else
                ++m_counter;
        }

        T NewId() { Increase(); return m_counter;}
        T getCurrent() const { return m_counter;}

        static const T Limit = limit;

    private:
        void init() { m_counter = 0; }
        T m_counter;
    };

    typedef counter<uint32, 0xFFFFFFFF> UInt32Counter;
}
