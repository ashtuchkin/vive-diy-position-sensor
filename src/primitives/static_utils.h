// This file contains a collection of useful compile-time constructs.
#pragma once

// Simple version of static asserts. For use if C++11 is not available (otherwise, use static_assert()).
template <bool b> struct StaticAssert;
template <>       struct StaticAssert<true>{ enum { value = 1 }; };
template <int a, int b> struct StaticAssertEqual;
template <int a>        struct StaticAssertEqual<a, a>{ enum { value = 1 }; };
#define _STATIC_JOIN_(x, y) x ## y
#define _STATIC_JOIN(x, y) _STATIC_JOIN_(x, y)

#define STATIC_ASSERT(B) enum { _STATIC_JOIN(static_assert_, __LINE__) = sizeof(StaticAssert<(bool)( B )>) }
#define STATIC_ASSERT_EQUAL(A, B) enum { _STATIC_JOIN(static_assert_, __LINE__) = sizeof(StaticAssertEqual<(A), (B)>) }

