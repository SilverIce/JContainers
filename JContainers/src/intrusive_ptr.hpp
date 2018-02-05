#pragma once
//
//  intrusive_ptr_jc.hpp
//
//  Copyright (c) 2001, 2002 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/intrusive_ptr_jc.html for documentation.
//

#include <boost/config.hpp>

#include <boost/assert.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/smart_ptr/detail/sp_convertible.hpp>
#include <boost/smart_ptr/detail/sp_nullptr_t.hpp>
#include <boost/smart_ptr/detail/sp_noexcept.hpp>

#include <boost/config/no_tr1/functional.hpp>           // for std::less

#if !defined(BOOST_NO_IOSTREAM)
#if !defined(BOOST_NO_IOSFWD)
#include <iosfwd>               // for std::basic_ostream
#else
#include <ostream>
#endif
#endif


namespace boost
{

//
//  intrusive_ptr_jc
//
//  A smart pointer that uses intrusive reference counting.
//
//  Relies on unqualified calls to
//  
//      void intrusive_ptr_add_ref(T * p);
//      void intrusive_ptr_release(T * p);
//
//          (p != 0)
//
//  The object is responsible for destroying itself.
//

template<class T, class Policy> class intrusive_ptr_jc
{
private:

    typedef intrusive_ptr_jc this_type;

public:

    typedef T element_type;

    intrusive_ptr_jc() BOOST_NOEXCEPT : px(0)
    {
    }

    intrusive_ptr_jc(T * p, bool add_ref = true) : px(p)
    {
        if (px != 0 && add_ref) Policy::retain(px);
    }

    intrusive_ptr_jc(T & p, bool add_ref = true) : px(&p)
    {
        if (add_ref) Policy::retain(px);
    }

#if !defined(BOOST_NO_MEMBER_TEMPLATES) || defined(BOOST_MSVC6_MEMBER_TEMPLATES)

    template<class U, class UP>
#if !defined( BOOST_SP_NO_SP_CONVERTIBLE )

    intrusive_ptr_jc(intrusive_ptr_jc<U,UP> const & rhs, typename boost::detail::sp_enable_if_convertible<U, T>::type = boost::detail::sp_empty())

#else

    intrusive_ptr_jc( intrusive_ptr_jc<U,UP> const & rhs )

#endif
    : px( rhs.get() )
    {
        if (px != 0) Policy::retain(px);
    }

#endif

    intrusive_ptr_jc(intrusive_ptr_jc const & rhs) : px(rhs.px)
    {
        if (px != 0) Policy::retain(px);
    }

    ~intrusive_ptr_jc()
    {
        if (px != 0) Policy::release(px);
    }

    template<class Base>
    intrusive_ptr_jc<Base, Policy>& to_base() {
        static_assert(std::is_base_of<Base, T>::value, "");
        return reinterpret_cast< intrusive_ptr_jc<Base, Policy>& >(*this);
    }

    template<class Base>
    const intrusive_ptr_jc<Base, Policy>& to_base() const {
        static_assert(std::is_base_of<Base, T>::value, "");
        return reinterpret_cast<const intrusive_ptr_jc<Base, Policy>& >(*this);
    }

#if !defined(BOOST_NO_MEMBER_TEMPLATES) || defined(BOOST_MSVC6_MEMBER_TEMPLATES)

    template<class U, class UP> intrusive_ptr_jc & operator=(intrusive_ptr_jc<U,UP> const & rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

#endif

// Move support

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

    intrusive_ptr_jc(intrusive_ptr_jc && rhs) BOOST_NOEXCEPT : px(rhs.px)
    {
        rhs.px = 0;
    }

    intrusive_ptr_jc & operator=(intrusive_ptr_jc && rhs) BOOST_NOEXCEPT
    {
        this_type(static_cast< intrusive_ptr_jc && >(rhs)).swap(*this);
        return *this;
    }

#endif

    intrusive_ptr_jc & operator=(intrusive_ptr_jc const & rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

    intrusive_ptr_jc & operator=(T * rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

    void reset() BOOST_NOEXCEPT
    {
        this_type().swap( *this );
    }

    void reset( T * rhs )
    {
        this_type( rhs ).swap( *this );
    }

    T * get() const BOOST_NOEXCEPT
    {
        return px;
    }

    T & operator*() const
    {
        BOOST_ASSERT( px != 0 );
        return *px;
    }

    T * operator->() const
    {
        //BOOST_ASSERT( px != 0 );
        return px;
    }

// implicit conversion to "bool"
#include <boost/smart_ptr/detail/operator_bool.hpp>

    void swap(intrusive_ptr_jc & rhs) BOOST_NOEXCEPT
    {
        T * tmp = px;
        px = rhs.px;
        rhs.px = tmp;
    }

    // JContainers addition that silently sets pointer to zero without releasing it
    void jc_nullify()
    {
        px = nullptr;
    }

    operator T * () const {
        return px;
    }

private:

    T * px;
};

template<class T, class TP, class U, class UP> inline bool operator==(intrusive_ptr_jc<T,TP> const & a, intrusive_ptr_jc<U,UP> const & b)
{
    return a.get() == b.get();
}

template<class T, class TP, class U, class UP> inline bool operator!=(intrusive_ptr_jc<T,TP> const & a, intrusive_ptr_jc<U,UP> const & b)
{
    return a.get() != b.get();
}

template<class T, class TP, class U, class UP> inline bool operator==(intrusive_ptr_jc<T,TP> const & a, U * b)
{
    return a.get() == b;
}

template<class T, class TP, class U, class UP> inline bool operator!=(intrusive_ptr_jc<T,TP> const & a, U * b)
{
    return a.get() != b;
}

template<class T, class TP, class U, class UP> inline bool operator==(T * a, intrusive_ptr_jc<U,UP> const & b)
{
    return a == b.get();
}

template<class T, class TP, class U, class UP> inline bool operator!=(T * a, intrusive_ptr_jc<U,UP> const & b)
{
    return a != b.get();
}

#if __GNUC__ == 2 && __GNUC_MINOR__ <= 96

// Resolve the ambiguity between our op!= and the one in rel_ops

template<class T, class TP> inline bool operator!=(intrusive_ptr_jc<T,TP> const & a, intrusive_ptr_jc<T,TP> const & b)
{
    return a.get() != b.get();
}

#endif

#if !defined( BOOST_NO_CXX11_NULLPTR )

template<class T, class TP> inline bool operator==( intrusive_ptr_jc<T,TP> const & p, boost::detail::sp_nullptr_t ) BOOST_NOEXCEPT
{
    return p.get() == 0;
}

template<class T, class TP> inline bool operator==( boost::detail::sp_nullptr_t, intrusive_ptr_jc<T,TP> const & p ) BOOST_NOEXCEPT
{
    return p.get() == 0;
}

template<class T, class TP> inline bool operator!=( intrusive_ptr_jc<T,TP> const & p, boost::detail::sp_nullptr_t ) BOOST_NOEXCEPT
{
    return p.get() != 0;
}

template<class T, class TP> inline bool operator!=( boost::detail::sp_nullptr_t, intrusive_ptr_jc<T,TP> const & p ) BOOST_NOEXCEPT
{
    return p.get() != 0;
}

#endif

template<class T, class TP> inline bool operator<(intrusive_ptr_jc<T,TP> const & a, intrusive_ptr_jc<T,TP> const & b)
{
    return std::less<T *>()(a.get(), b.get());
}

template<class T, class TP> void swap(intrusive_ptr_jc<T,TP> & lhs, intrusive_ptr_jc<T,TP> & rhs)
{
    lhs.swap(rhs);
}

// mem_fn support

template<class T, class TP> T * get_pointer(intrusive_ptr_jc<T, TP> const & p)
{
    return p.get();
}

template<class T, class TP, class U, class UP> intrusive_ptr_jc<T,TP> static_pointer_cast(intrusive_ptr_jc<U,UP> const & p)
{
    return static_cast<T *>(p.get());
}

template<class T, class TP, class U, class UP> intrusive_ptr_jc<T, TP> const_pointer_cast(intrusive_ptr_jc<U, UP> const & p)
{
    return const_cast<T *>(p.get());
}

template<class T, class TP, class U, class UP> intrusive_ptr_jc<T, TP> dynamic_pointer_cast(intrusive_ptr_jc<U, UP> const & p)
{
    return dynamic_cast<T *>(p.get());
}

// operator<<

#if !defined(BOOST_NO_IOSTREAM)

#if defined(BOOST_NO_TEMPLATED_IOSTREAMS) || ( defined(__GNUC__) &&  (__GNUC__ < 3) )

template<class Y, class YP> std::ostream & operator<< (std::ostream & os, intrusive_ptr_jc<Y,YP> const & p)
{
    os << p.get();
    return os;
}

#else

// in STLport's no-iostreams mode no iostream symbols can be used
#ifndef _STLP_NO_IOSTREAMS

# if defined(BOOST_MSVC) && BOOST_WORKAROUND(BOOST_MSVC, < 1300 && __SGI_STL_PORT)
// MSVC6 has problems finding std::basic_ostream through the using declaration in namespace _STL
using std::basic_ostream;
template<class E, class T, class Y> basic_ostream<E, T> & operator<< (basic_ostream<E, T> & os, intrusive_ptr_jc<Y> const & p)
# else
template<class E, class T, class Y, class YP> std::basic_ostream<E, T> & operator<< (std::basic_ostream<E, T> & os, intrusive_ptr_jc<Y,YP> const & p)
# endif 
{
    os << p.get();
    return os;
}

#endif // _STLP_NO_IOSTREAMS

#endif // __GNUC__ < 3

#endif // !defined(BOOST_NO_IOSTREAM)

// hash_value

template< class T > struct hash;

template< class T, class TP > std::size_t hash_value( boost::intrusive_ptr_jc<T,TP> const & p )
{
    return boost::hash< T* >()( p.get() );
}

} // namespace boost
