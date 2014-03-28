#ifndef MANTA_CORE_UTIL_PREPROCESSOR_H_
#define MANTA_CORE_UTIL_PREPROCESSOR_H_

#if defined(__GNUC__)
#define MANTA_FUNC __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define MANTA_FUNC __FUNCTION__ // or perhaps __FUNCDNAME__ + __FUNCSIG__
#else
#define MANTA_FUNC __func__
#endif

#include <stdlib.h>

#ifdef _MSC_VER
# include <malloc.h>
# define alloca  _alloca
#else
# include <alloca.h>
#endif

// NOTE(boulos): ISO C99 defines _Pragma to let you do this.
#define MANTA_PRAGMA(str) _Pragma (#str)

// Even with the Intel Compiler and Qstd=c99, this won't work on
// Win32...
#if defined(__INTEL_COMPILER) && !defined(_WIN32)
#define MANTA_UNROLL(amt) MANTA_PRAGMA(unroll((amt)))
#else
// NOTE(boulos): Assuming GCC
#define MANTA_UNROLL(unroll_amount)
#endif

#if defined(__INTEL_COMPILER)
#define MANTA_FORCEINLINE __forceinline
#else
// NOTE(boulos): Assuming GCC
#define MANTA_FORCEINLINE __attribute__ ((always_inline))
#endif

// Stack allocation equivalent to type[size]
#define MANTA_STACK_ALLOC(type, size) ((type*)alloca((size) * sizeof(type)))

#if defined(_WIN32)
#define MANTA_PLUGINEXPORT extern "C" __declspec(dllexport)
#else
#define MANTA_PLUGINEXPORT extern "C"
#endif

#endif // MANTA_CORE_UTIL_MANTA_PREPROCESSOR_H_
