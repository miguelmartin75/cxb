#ifndef CXB_C_H
#define CXB_C_H

#include <stdatomic.h> // _Atomic(T)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // atof
#include <string.h>
#define CXB_EXPORT
#define CXB_INTERNAL static
#define COUNTOF_LIT(a) (size_t) (sizeof(a) / sizeof(*(a)))
#define LENGTHOF_LIT(s) (COUNTOF_LIT(s) - 1)

// see: https://github.com/EpicGamesExt/raddebugger/blob/master/src/base/base_core.h
#define BYTES(n) ((u64) (n))
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
#if defined(__clang__) || defined(__GNUC__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

/* Platform detection */
#if defined(__APPLE__)
#define CXB_PLATFORM_DARWIN 1
#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
#define CXB_PLATFORM_MACOS 1
#endif
#if defined(TARGET_OS_IOS) && TARGET_OS_IOS
#define CXB_PLATFORM_IOS 1
#endif
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define CXB_PLATFORM_IPHONE 1
#endif
#if defined(TARGET_OS_IPAD) && TARGET_OS_IPAD
#define CXB_PLATFORM_IPAD 1
#endif
#if defined(TARGET_OS_WATCH) && TARGET_OS_WATCH
#define CXB_PLATFORM_APPLE_WATCH 1
#endif
#endif

#if defined(__linux__)
#define CXB_PLATFORM_LINUX 1
#endif

#if defined(_WIN32) || defined(_WIN64)
#define CXB_PLATFORM_WINDOWS 1
#endif

#if defined(CXB_PLATFORM_LINUX) || defined(CXB_PLATFORM_DARWIN)
#define CXB_PLATFORM_UNIX 1
#endif

#define INTERNAL static
#define GLOBAL static

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
void __asan_poison_memory_region(void const volatile* addr, size_t size);
void __asan_unpoison_memory_region(void const volatile* addr, size_t size);
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

typedef uint8_t byte8;
// TODO
// typedef _Float16 f16;
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

#if defined(__STDC_NO_ATOMICS__)
#error "C11 _Atomic is not available, compile with C++23"
#endif

#if defined(__GNUC__)
#define CXB_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
#define CXB_INLINE __forceinline
#else
#define CXB_INLINE inline
#endif

struct ArenaParams {
    size_t reserve_bytes;
    size_t max_n_blocks;
};

struct Arena {
    char* start;
    char* end;
    size_t pos;
    ArenaParams params;

    // chaining
    Arena* next;
    Arena* prev;
    size_t n_blocks;

    // TODO: freelist
};

struct ArenaTmp {
    Arena* arena;
    u64 pos;
};

struct Allocator {
    void* (*alloc_proc)(void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes, bool fill_zeros, void* data);
    void (*free_proc)(void* head, size_t n_bytes, void* data);
    void (*free_all_proc)(void* data);
    void* data;
};
extern Allocator heap_alloc;

struct String8 {
    char* data;
    union {
        struct {
            size_t len : 63;
            bool not_null_term : 1;
        };
        size_t metadata;
    };
};

struct String8SplitIterator {
    String8 s;
    String8 delim;
    u64 pos;
    String8 curr;
    bool any;
};

struct Vec2f {
    f32 x, y;
};

struct Vec2i {
    i32 x, y;
};

struct Size2i {
    i32 w, h;
};

struct Vec3f {
    f32 x, y, z;
};

struct Vec3i {
    i32 x, y, z;
};

struct Rect2f {
    f32 x, y;
    f32 w, h;
};

struct Rect2ui {
    u32 x, y;
    u32 w, h;
};

struct Color4f {
    f32 r, g, b, a;
};

struct Color4i {
    byte8 r, g, b, a;
};

struct Mat33f {
    f32 arr[9];
};

struct Mat44f {
    f32 arr[16];
};

struct MString8 {
    char* data;
    union {
        struct {
            size_t len : 63;
            bool not_null_term : 1;
        };
        size_t metadata;
    };
    size_t capacity;
    Allocator* allocator;
};
#define S8_LIT(s) (String8{.data = (char*) &(s)[0], .len = LENGTHOF_LIT(s), .not_null_term = false})
#define S8_DATA(c, l) (String8{.data = (char*) &(c)[0], .len = (l), .not_null_term = false})
#define S8_CSTR(s) (String8{.data = (char*) (s), .len = (size_t) strlen(s), .not_null_term = false})

struct Utf8Iter {
    String8 s;
    u64 pos;
};

struct Utf8IterBatch {
    u32 data[512];
    u64 len;
};

#endif /* CXB_C_H */
