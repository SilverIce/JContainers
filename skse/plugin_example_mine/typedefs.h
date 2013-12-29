
/**
  file:         typedefs.h
  author:       SilverIce
  email:        slifeleaf@gmail.com
  created:      16:2:2011
*/

#pragma once

namespace G3D
{
    class Vector2;
    class Vector3;
    class Vector4;
}

/*  TODO: хидер может использоваться в 2х прожектах используемом и клиентом
    в этом случае классы используемого прожекта-библиотеки будут видимы с __declspec(dllimport) атрибутом.

*/
#   ifdef EXPORTING
#       define EXPORT __declspec(dllexport)
#   else
#       define EXPORT __declspec(dllimport)
#   endif

//#define EXPORT __declspec( dllexport )
//#define IMPORT __declspec( dllimport )

#ifndef CDECL
#define CDECL  __cdecl
#endif

namespace Movement
{
    typedef signed char     int8;
    typedef unsigned char   uint8;
    typedef short           int16;
    typedef unsigned short  uint16;
    typedef int             int32;
    typedef unsigned int    uint32;
    typedef __int64         int64;
    typedef unsigned __int64 uint64;

    using G3D::Vector2;
    using G3D::Vector3;
    using G3D::Vector4;
}
