#pragma once

#include <string>
#include "skse/Utilities.h"

namespace skse {
    
    class string_ref {
        const char* data = nullptr;

        void assign(const char *buf) {
            if (buf) {
                CALL_MEMBER_FN(this, Set)(buf);
            } 
            else {
                release();
            }
        }

        void release() {
            if (data) {
                CALL_MEMBER_FN(this, Release)();
                data = nullptr;
            }
        }

        MEMBER_FN_PREFIX(string_ref);
        DEFINE_MEMBER_FN(ctor, string_ref *, 0x00A511C0, const char * buf);
        DEFINE_MEMBER_FN(Set, string_ref *, 0x00A51210, const char * buf);
        DEFINE_MEMBER_FN(Release, void, 0x00A511B0);

    public:

        string_ref() { }

        string_ref(const char * buf) {
            CALL_MEMBER_FN(this, ctor)(buf);
        }

        template<class Tr, class Alloc>
        string_ref(const std::basic_string<char, Tr, Alloc>& string) {
            CALL_MEMBER_FN(this, ctor)(string.c_str());
        }

        ~string_ref() {
            release();
        }

        string_ref(const string_ref& ref) { CALL_MEMBER_FN(this, ctor)(ref.data); }

        string_ref& operator = (const string_ref& ref) { assign(ref.data); return *this; }

        string_ref& operator = (const char* ref) { assign(ref); return *this; }

        template<class Tr, class Alloc>
        string_ref& operator = (const std::basic_string<char, Tr, Alloc>& string) {
            assign(string.c_str());
            return *this;
        }

        string_ref(string_ref&& ref) {
            std::swap(data, ref.data);
        }

        string_ref& operator = (string_ref&& ref) {
            release();
            std::swap(data, ref.data);
            return *this;
        }

        bool operator==(const string_ref& lhs) const { return data == lhs.data; }
        //bool operator<(const sting_ref& lhs) const { return data < lhs.data; }

        const char* c_str() const {
            return data;
        }
    };
}
