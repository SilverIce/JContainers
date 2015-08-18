#pragma once

#include <boost/variant.hpp>
#include <string>

#include "common/ITypes.h"
#include "object/object_base.h"
#include "skse/skse.h"
#include "skse/string.h"

#include "collections/collections.h"
#include "collections/types.h"
#include "collections/dyn_form_watcher.h"


namespace collections {

    enum item_type {
        no_item = 0,
        none,
        integer,
        real,
        form,
        object,
        string,
    };

    class item {
    public:
        typedef boost::blank blank;
        typedef Float32 Real;
        typedef boost::variant<boost::blank, SInt32, Real, weak_form_id, internal_object_ref, std::string> variant;

    private:
        variant _var;

    private:

        template<class T> struct type2index{ };

        template<> struct type2index < boost::blank >  { static const item_type index = none; };
        template<> struct type2index < SInt32 >  { static const item_type index = integer; };
        template<> struct type2index < Real >  { static const item_type index = real; };
        template<> struct type2index < weak_form_id >  { static const item_type index = form; };
        template<> struct type2index < internal_object_ref >  { static const item_type index = object; };
        template<> struct type2index < std::string >  { static const item_type index = string; };

        static_assert(type2index<Real>::index > type2index<SInt32>::index, "Item::type2index works incorrectly");

    public:

        void u_nullifyObject() {
            if (auto ref = boost::get<internal_object_ref>(&_var)) {
                ref->jc_nullify();
            }
        }

        item() {}
        item(item&& other) : _var(std::move(other._var)) {}
        item(const item& other) : _var(other._var) {}

        item& operator = (item&& other) {
            _var = std::move(other._var);
            return *this;
        }

        item& operator = (const item& other) {
            _var = other._var;
            return *this;
        }

        const variant& var() const {
            return _var;
        }

        variant& var() {
            return _var;
        }

        template<class T> bool is_type() const {
            return boost::get<T>(&_var) != nullptr;
        }

        item_type type() const {
            return item_type(_var.which() + 1);
        }

        template<class T> T* get() {
            return boost::get<T>(&_var);
        }

        template<class T> const T* get() const {
            return boost::get<T>(&_var);
        }

        //////////////////////////////////////////////////////////////////////////

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const;
        template<class Archive>
        void load(Archive & ar, const unsigned int version);

        //////////////////////////////////////////////////////////////////////////


        explicit item(Real val) : _var(val) {}
        explicit item(double val) : _var((Real)val) {}
        explicit item(SInt32 val) : _var(val) {}
        explicit item(int val) : _var((SInt32)val) {}
        explicit item(bool val) : _var((SInt32)val) {}
        explicit item(const weak_form_id& id) : _var(id) {}
        explicit item(object_base& o) : _var(o) {}

        explicit item(const std::string& val) : _var(val) {}
        explicit item(std::string&& val) : _var(val) {}

        // the Item is none if the pointers below are zero:
        explicit item(const char * val) {
            *this = val;
        }
        explicit item(const skse::string_ref& val) {
            *this = val.c_str();
        }
        explicit item(object_base *val) {
            *this = val;
        }
        explicit item(const object_stack_ref &val) {
            *this = val.get();
        }
        /*
        explicit Item(const BSFixedString& val) : _var(boost::blank()) {
        *this = val.data;
        }*/

        item& operator = (unsigned int val) { _var = (SInt32)val; return *this; }
        item& operator = (int val) { _var = (SInt32)val; return *this; }
        item& operator = (bool val) { _var = (SInt32)val; return *this; }
        item& operator = (SInt32 val) { _var = val; return *this; }
        item& operator = (Real val) { _var = val; return *this; }
        item& operator = (double val) { _var = (Real)val; return *this; }
        item& operator = (const std::string& val) { _var = val; return *this; }
        item& operator = (std::string&& val) { _var = val; return *this; }
        item& operator = (boost::blank) { _var = boost::blank(); return *this; }
        item& operator = (boost::none_t) { _var = boost::blank(); return *this; }
        item& operator = (object_base& v) { _var = &v; return *this; }


/*
        item& operator = (FormId formId) {
            // prevent zero FormId from being saved
            if (formId) {
                _var = formId;
            }
            else {
                _var = blank();
            }
            return *this;
        }*/

        template<class T>
        item& _assignPtr(T *ptr) {
            if (ptr) {
                _var = ptr;
            }
            else {
                _var = blank();
            }
            return *this;
        }

        item& operator = (const char *val) {
            return _assignPtr(val);
        }

        item& operator = (object_base *val) {
            return _assignPtr(val);
        }

        object_base *object() const {
            if (auto ref = boost::get<internal_object_ref>(&_var)) {
                return ref->get();
            }
            return nullptr;
        }

        Real fltValue() const {
            if (auto val = boost::get<item::Real>(&_var)) {
                return *val;
            }
            else if (auto val = boost::get<SInt32>(&_var)) {
                return *val;
            }
            return 0;
        }

        SInt32 intValue() const {
            if (auto val = boost::get<SInt32>(&_var)) {
                return *val;
            }
            else if (auto val = boost::get<item::Real>(&_var)) {
                return *val;
            }
            return 0;
        }

        const char * strValue() const {
            if (auto val = boost::get<std::string>(&_var)) {
                return val->c_str();
            }
            return nullptr;
        }

        std::string * stringValue() {
            return boost::get<std::string>(&_var);
        }

        TESForm * form() const {
            auto frmId = formId();
            return frmId != FormZero ? skse::lookup_form(frmId) : nullptr;
        }

        FormId formId() const {
            if (auto val = boost::get<weak_form_id>(&_var)) {
                return val->get();
            }
            return FormZero;
        }

        class are_strict_equals : public boost::static_visitor<bool> {
        public:

            template <typename T, typename U>
            bool operator()(const T &, const U &) const {
                return false; // cannot compare different types
            }

            template <typename T>
            bool operator()(const T & lhs, const T & rhs) const {
                return lhs == rhs;
            }

            template <> bool operator()(const std::string & lhs, const std::string & rhs) const {
                return _stricmp(lhs.c_str(), rhs.c_str()) == 0;
            }
        };

        bool isEqual(const item& other) const {
            return boost::apply_visitor(are_strict_equals(), _var, other._var);
        }

        bool isNull() const {
            return is_type<boost::blank>();
        }

        bool isNumber() const {
            return is_type<SInt32>() || is_type<Real>();
        }

        template<class T> T readAs();

        //////////////////////////////////////////////////////////////////////////
    private:
        static_assert(std::is_same<
            boost::variant<boost::blank, SInt32, Real, weak_form_id, internal_object_ref, std::string>,
            variant
        >::value, "update _user2variant code below");

        // maps input user type to variant type:
        template<class T> struct _user2variant { using variant_type = T; };
        template<class V> struct _variant_type { using variant_type = V; };

        template<> struct _user2variant<uint32_t> : _variant_type<SInt32>{};
        template<> struct _user2variant<int32_t> : _variant_type<SInt32>{};
        template<> struct _user2variant<bool> : _variant_type<SInt32>{};

        template<> struct _user2variant<double> : _variant_type<Real>{};

        template<> struct _user2variant<char*> : _variant_type<std::string>{};
        template<size_t N> struct _user2variant<char[N]> : _variant_type<std::string>{};
        template<> struct _user2variant<char[]> : _variant_type<std::string>{};

        template<> struct _user2variant<const object_base*> : _variant_type<internal_object_ref>{};

    public:

        bool operator == (const item& other) const { return isEqual(other); }

        bool operator == (const object_base &obj) const { return *this == &obj; }

        template<class T>
        bool operator == (const T& v) const {
            auto thisV = boost::get<typename _user2variant<std::remove_const<T>::type>::variant_type>(&_var);
            return thisV && *thisV == v;
        }

        template<class T>
        bool operator != (const T& v) const { return !(*this == v); }

        //////////////////////////////////////////////////////////////////////////

        bool operator < (const item& other) const {
            const auto l = type(), r = other.type();
            return l == r ? boost::apply_visitor(lesser_comparison(), _var, other._var) : (l < r);
        }
    private:
        class lesser_comparison : public boost::static_visitor < bool > {
        public:

            template <typename T, typename U>
            bool operator()(const T &, const U &) const {
                return type2index<T>::index < type2index<U>::index;
            }

            template <typename T>
            bool operator()(const T & lhs, const T & rhs) const {
                return lhs < rhs;
            }

            bool operator()(const std::string & lhs, const std::string & rhs) const {
                return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
            }

        };

    };

    template<> inline item::Real item::readAs<item::Real>() {
        return fltValue();
    }

    template<> inline SInt32 item::readAs<SInt32>() {
        return intValue();
    }

    template<> inline const char * item::readAs<const char *>() {
        return strValue();
    }

    template<> inline std::string item::readAs<std::string>() {
        auto str = stringValue();
        return str ? *str : std::string();
    }

    template<> inline skse::string_ref item::readAs<skse::string_ref>() {
        const char *chr = strValue();
        return chr ? skse::string_ref(chr) : skse::string_ref();
    }

    template<> inline Handle item::readAs<Handle>() {
        auto obj = object();
        return obj ? obj->uid() : HandleNull;
    }

    template<> inline TESForm * item::readAs<TESForm*>() {
        return form();
    }

    template<> inline object_base * item::readAs<object_base*>() {
        return object();
    }
}