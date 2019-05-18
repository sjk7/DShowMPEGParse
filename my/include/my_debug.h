#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "my_cpp_versions.hpp"
#ifndef ASSERT
#include <cassert>
#define ASSERT assert
#endif

#ifndef TRACE
#if defined _DEBUG || !defined NDEBUG || defined DEBUG
#define DEBUGGABLE
#endif
#endif

namespace my {
namespace detail {
    static inline int my_trace(const char* fmt, ...) {
        va_list myargs;
        va_start(myargs, fmt);

        int ret = vprintf(fmt, myargs);

        /* Clean up the va_list */
        va_end(myargs);
        // ret += printf("%s", "\r\n");
        fflush(stdout);
        return ret;
    } // my_trace
} // namespace detail
} // namespace my

namespace my {
namespace detail {
#if CPP <= 9

    template <typename T>
    inline void pt(const T& t, bool is_first, bool is_last = false) {
        if (!is_first) {
            std::cout << " " << t;
        } else {
            std::cout << t;
        }
        if (is_last) std::cout << std::endl;
    }
    template <typename A> static inline int my_print(const A& a) {
        pt(a, true, true);
        return 0;
    }

    template <typename A, typename B>
    static inline int my_print(const A& a, const B& b) {
        pt(a, true);
        pt(b, false, true);
        return 0;
    }

    template <typename A, typename B, typename C>
    static inline int my_print(const A& a, const B& b, const C& c) {
        pt(a, true);
        pt(b, false);
        pt(c, false, true);
        return 0;
    }
    template <typename A, typename B, typename C, typename D>
    static inline int my_print(const A& a, const B& b, const C& c, const D& d) {
        pt(a, true);
        pt(b, false);
        pt(c, false);
        pt(d, false, true);
        return 0;
    }

    template <typename A, typename B, typename C, typename D, typename E>
    static inline int my_print(
        const A& a, const B& b, const C& c, const D& d, const E& e) {
        pt(a, true);
        pt(b, false);
        pt(c, false);
        pt(d, false);
        pt(e, false, true);
        return 0;
    }

    template <typename A, typename B, typename C, typename D, typename E,
        typename F>
    static inline int my_print(const A& a, const B& b, const C& c, const D& d,
        const E& e, const F& f) {
        pt(a, true);
        pt(b, false);
        pt(c, false);
        pt(d, false);
        pt(e, false);
        pt(f, false, true);
        return 0;
    }

    template <typename A, typename B, typename C, typename D, typename E,
        typename F, typename G>
    static inline int my_print(const A& a, const B& b, const C& c, const D& d,
        const E& e, const F& f, const G& g) {
        pt(a, true);
        pt(b, false);
        pt(c, false);
        pt(d, false);
        pt(e, false);
        pt(f, false);
        pt(g, false, true);
        return 0;
    }

#if CPP >= 11
    namespace detail {
        template <typename... Ts> constexpr void my_print(Ts&&... args) {
            const int dummy[]
                = {0, (std::cout << std::forward<Ts>(args), 0)...};
            static_cast<void>(dummy); // avoid warning for unused variable
            std::cout << std::endl;
            return;
        }
    } // namespace detail
#define PRINT my::detail::my_print
#define SHOW my::detail::my_print
#define TRACE my::detail::my_trace
#else
    //#error "my print not available until c++11"

#endif // #if CPP >=11

} // namespace detail

#endif // CPP >=9

#if CPP >= 17
namespace detail {
    template <typename T, typename... Ts>
    constexpr inline void my_print(T&& first, Ts&&... args) noexcept {
        std::cout << first;
        ((std::cout << ' ' << args), ...);
        std::cout << std::endl;
    }
} // namespace detail
#endif

} // namespace my

#define TRACE my::detail::my_trace
#define SHOW my::detail::my_print
#define PRINT my::detail::my_print
