// my_cpp_versions.hpp
#pragma once
/*/

    C++ pre-C++98: __cplusplus is 1.
    C++98: __cplusplus is 199711L.
    C++98 + TR1: This reads as C++98 and there is no way to check that I know
of. C++11: __cplusplus is 201103L. C++14: __cplusplus is 201402L. C++17:
__cplusplus is 201703L.

/*/
#ifndef CPP

#ifndef __cplusplus
#error "Not a recognised c++ compiler"
#endif

#if __cplusplus <= 199711L
#define CPP 9
#ifndef constexpr
#define constexpr
#endif

#ifndef noexcept
#define noexcept
#endif

#ifndef nullptr
#define nullptr 0
#endif
#endif

#if __cplusplus >= 201103L
#define CPP 11
#endif

#if __cplusplus >= 201402L
#undef CPP
#define CPP 14
#endif

#if __cplusplus == 201703L
#undef CPP
#define CPP 17
#endif

#if __cplusplus > 201703L
#undef CPP
#define CPP 20
#endif

#endif

#ifdef _MSC_VER
#define DOZE
#endif

#ifdef __linux__
#define LINUX
#endif

#ifdef __clang__
#define CLANG
#endif

#ifdef __APPLE__
#define APPLE
#endif
