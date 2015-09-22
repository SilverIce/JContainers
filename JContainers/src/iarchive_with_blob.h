#pragma once
#include <boost/archive/binary_iarchive.hpp>
#include <type_traits>
#include <tuple>
#include <assert.h>

namespace collections {
    class tes_context;
    class object_context;
}

namespace hack {

    using iarchive_with_blob_base = boost::archive::binary_iarchive;

    //boost::archive::binary_iarchive
    // To carry all necessay information needed to initialize objects from older archive formats
    // Should be replaced with boost::archive::binary_iarchive when this class won't be needed
    template<class T0, class T1>
    class iarchive_with_blob_templ : public iarchive_with_blob_base {
        std::tuple<T0&,T1&> _blob;
    public:

        template<class T> T& get() {
            return std::get<T&>(_blob);
        }

        template<> T0& get<T0>() {
            return std::get<0>(_blob);
        }

        template<> T1& get<T1>() {
            return std::get<1>(_blob);
        }

        iarchive_with_blob_templ() = delete;

        template<class ...Types>
        explicit iarchive_with_blob_templ(std::istream & stream, Types& ...args)
            : iarchive_with_blob_base(stream)
            , _blob(args ...)
        {}

        template<class T>
        static T& from_base_get(iarchive_with_blob_base& base) {
            auto blob = dynamic_cast<iarchive_with_blob_templ*>(&base);
            assert(blob);
            return blob->get<T>();
        }
    };

    using iarchive_with_blob = iarchive_with_blob_templ < collections::tes_context, collections::object_context > ;

    static_assert(std::has_virtual_destructor<boost::archive::binary_iarchive>::value, "it MUST implement virtual destructor");
    static_assert(std::is_base_of<boost::archive::binary_iarchive, iarchive_with_blob>::value, "");

}
/*

BOOST_SERIALIZATION_REGISTER_ARCHIVE(hack::iarchive_with_blob);
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(hack::iarchive_with_blob);*/
