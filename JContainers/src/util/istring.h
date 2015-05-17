#pragma once

#include <string.h>

namespace util {

    struct istring_traits : public std::char_traits<char> {

        static bool eq(char c1, char c2) {
            return tolower(c1) == tolower(c2);
        }
        static bool lt(char c1, char c2) {
            return tolower(c1) < tolower(c2);
        }
        static int compare(const char* s1, const char* s2, size_t n) {
            return n > 0 ? _stricmp(s1, s2) : 0;
        }
        static const char* find(const char* s, int n, char a) {
            while (n-- > 0 && tolower(*s) != tolower(a)) {
                ++s;
            }
            return s;
        }
        static bool eq_int_type(const int_type& _Left, const int_type& _Right)
        {	// test for metacharacter equality
            return (tolower(_Left) == tolower(_Right));
        }
    };

    typedef std::basic_string<char, istring_traits, std::allocator<char> > istring;
}
