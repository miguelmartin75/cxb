#ifndef CXB_C_H
#define CXB_C_H

// NOTE: to generate the C header file
#define CXB_C_COMPAT_BEGIN
#define CXB_C_COMPAT_END

/* SECTION: includes */
CXB_C_COMPAT_BEGIN

#include <stdatomic.h> // _Atomic(T)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* SECTION: macros */
// NOTE some macros are copied/modified from Blend2D, see:
// https://github.com/blend2d/blend2d/blob/bae3b6c600186a69a9f212243ed9700dc93a314a/src/blend2d/api.h#L563
#define CXB_EXPORT
#define CXB_INTERNAL static
#define COUNTOF_LIT(a) (size_t) (sizeof(a) / sizeof(*(a)))
#define LENGTHOF_LIT(s) (COUNTOF_LIT(s) - 1)

// see: https://github.com/EpicGamesExt/raddebugger/blob/master/src/base/base_core.h
#define Bytes(n) ((u64) (n))
#define KB(n) ((u64) (n) << 10)
#define MB(n) ((u64) (n) << 20)
#define GB(n) ((u64) (n) << 30)
#define TB(n) ((u64) (n) << 40)
#define PB(n) ((u64) (n) << 50)

#define Thousands(x) ((x) * 1000LL)
#define Millions(x) (Thousands(x) * 1000LL)
#define Billions(x) (Millions(x) * 1000ULL)
#define Trillions(x) (Billions(x) * 1000ULL)

#if defined(__clang__)
#define BREAKPOINT() __builtin_debugtrap()
#elif defined(__GNUC__)
#define BREAKPOINT() __builtin_trap()
#else
#define BREAKPOINT() abort()
#endif

CXB_C_COMPAT_BEGIN

#define LIKELY(x) x
#define UNLIKELY(x) x

#define FN
#define CPOD
#define INTERNAL static
#define GLOBAL static

#ifdef __cplusplus
#define CFN extern "C"
#define CXB_C_EXPORT extern "C"
#define CXB_C_IMPORT extern "C"
#define C_DECL_BEGIN extern "C" {
#define C_DECL_END }
#else
#define CFN
#define CXB_C_EXPORT
#define CXB_C_IMPORT
#define C_DECL_BEGIN
#define C_DECL_END
#endif

#if defined(_MSC_VER)
#if defined(__SANITIZE_ADDRESS__)
#define ASAN_ENABLED 1
#define NO_ASAN __declspec(no_sanitize_address)
#else
#define NO_ASAN
#endif
#elif defined(__clang__)
#if defined(__has_feature)
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#define ASAN_ENABLED 1
#endif
#endif
#define NO_ASAN __attribute__((no_sanitize("address")))
#else
#define NO_ASAN
#endif

#if ASAN_ENABLED
CXB_C_IMPORT void __asan_poison_memory_region(void const volatile* addr, size_t size);
CXB_C_IMPORT void __asan_unpoison_memory_region(void const volatile* addr, size_t size);
#define ASAN_POISON_MEMORY_REGION(addr, size) __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) ((void) (addr), (void) (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void) (addr), (void) (size))
#endif

#define CXB_MAYBE_INLINE inline
#if defined(__GNUC__)
#define CXB_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
#define CXB_INLINE __forceinline
#else
#define CXB_INLINE inline
#endif

CXB_C_COMPAT_END

#define DEBUG_ASSERT(x, ...) \
    if(!(x)) BREAKPOINT()
#define ASSERT(x, ...)    \
    if(!(x)) BREAKPOINT()
#define INVALID_CODEPATH(msg) \
    /* TODO: output msg */    \
    BREAKPOINT()

/* * SECTION: primitives */
CXB_C_COMPAT_BEGIN
typedef uint8_t byte8;
typedef float f32;
typedef double f64;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t ll;
typedef int32_t rune;

#if defined(__GNUC__)
typedef __uint128_t u128;
typedef __int128_t i128;
// TODO: support MSVC
#endif

typedef _Atomic(i32) atomic_i32;
typedef _Atomic(i64) atomic_i64;
typedef _Atomic(u32) atomic_u32;
typedef _Atomic(u64) atomic_u64;
#if defined(__GNUC__)
typedef _Atomic(i128) atomic_i128;
typedef _Atomic(u128) atomic_u128;
#endif

/* SECTION: types */
typedef struct Allocator Allocator;
extern Allocator heap_alloc;

#define S8_LIT(s) (StringSlice{.data = (char*) &(s)[0], .len = LENGTHOF_LIT(s), .null_term = true})
#define S8_DATA(c, l) (StringSlice{.data = (char*) &(c)[0], .len = (l), .null_term = false})
#define S8_CSTR(s) (StringSlice{.data = (char*) (s), .len = (size_t) strlen(s), .null_term = true})

#define MSTRING_NT(a) (MString{.data = nullptr, .len = 0, .null_term = true, .capacity = 0, .allocator = (a)})
CXB_C_COMPAT_END
