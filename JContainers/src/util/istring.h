#pragma once

#include <string.h>

namespace util {

    class istring {
    public:
        using str = std::string;
        using elem = string::value_type;
        using selem = const elem*;

    private:
        str _str;

    public:

        istring() {}
        
        istring(const elem * s) : _str(s) {}
        istring(const string& s) : _str(s) {}
        istring(const istring& s) : _str(s._str) {}

        istring(istring&& s) : _str(std::move(s._str)) {}
        istring(string&& s) : _str(s) {}

        istring& operator = (const char* s) {
            _str = s;
            return *this;
        }
        istring& operator = (const istring& s) {
            _str = s._str;
            return *this;
        }
        istring& operator = (const string& s) {
            _str = s;
            return *this;
        }

        istring& operator = (istring&& s) {
            _str = std::move(s._str);
            return *this;
        }
        istring& operator = (string&& s) {
            _str = s;
            return *this;
        }

        selem c_str() const {
            return _str.c_str();
        }

        str& string() { return _str; }
        const str& string() const { return _str; }
    };

    namespace istring_comparison {

        namespace {
            template<class T>
            istring::selem get_char(const T& l) {
                return l.c_str();
            }
            istring::selem& get_char(istring::selem& l) {
                return l;
            }

            template<class T, class U>
            int compare(const T& l, const U& r) {
                return _stricmp(get_char(l), get_char(r));
            }
        }

        template<class T>
        bool operator == (const istring& l, const T& r) { return compare(l, r) == 0; }

        template<class T>
        bool operator != (const T& l, const istring& r) { return !(r == l); }

        //////////////////////////////////////////////////////////////////////////

        template<class T>
        bool operator < (const istring& l, const T& r) { return compare(l, r) < 0; }

        template<class T>
        bool operator < (const T& l, const istring& r) { return compare(l, r) < 0; }

        //////////////////////////////////////////////////////////////////////////

        template<class T>
        bool operator > (const istring& l, const T& r) { return compare(l, r) > 0; }

        template<class T>
        bool operator > (const T& l, const istring& r) { return compare(l, r) > 0; }

    }
}
