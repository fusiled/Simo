#ifndef SIMO_COMPILER_HH
#define SIMO_COMPILER_HH

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define SIMO_PUBLIC __attribute__((dllexport))
#else
#define SIMO_PUBLIC __declspec(dllexport)
#endif
#else
#define SIMO_PUBLIC __attribute__((visibility("default")))
#endif

#endif  // SIMO_COMPILER_HH

#ifndef NDEBUG
#include <cassert>
#define SIMO_ASSERT(...) assert(__VA_ARGS__)
#else
#define SIMO_ASSERT(...)
#endif