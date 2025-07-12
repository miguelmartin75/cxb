#ifndef CXB_C_H
#define CXB_C_H

/* SECTION: includes */
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

#if defined(__clang__)
#define BREAKPOINT() __builtin_debugtrap()
#elif defined(__GNUC__)
#define BREAKPOINT() __builtin_trap()
#else
#define BREAKPOINT() abort()
#endif

#define DEBUG_ASSERT(x, msg, ...) \
    if(!(x)) BREAKPOINT()
#define ASSERT(x, msg, ...) \
    if(!(x)) BREAKPOINT()
#define REQUIRES(x)       \
    if(!(x)) BREAKPOINT()
#define LIKELY(x) x
#define UNLIKELY(x) x

#ifdef __cplusplus
#define CXB_C_EXPORT extern "C"
#define CXB_C_IMPORT extern "C"
#else
#define CXB_C_EXPORT
#define CXB_C_IMPORT
#endif

#define CXB_MAYBE_INLINE inline
#if defined(__GNUC__)
#define CXB_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
#define CXB_INLINE __forceinline
#else
#define CXB_INLINE inline
#endif

/* * SECTION: primitives */
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
typedef struct Mallocator Mallocator;
extern Mallocator default_alloc;

#ifndef __cplusplus
typedef struct StringSlice {
    char* data;
    union {
        struct {
            size_t len : 63;
            bool null_term : 1;
        };
        size_t metadata;
    };
} StringSlice;

typedef struct MString {
    char* data;
    union {
        struct {
            size_t len : 63;
            bool null_term : 1;
        };
        size_t metadata;
    };
    size_t capacity;
    Allocator* allocator;
} MString;

/* SECTION: math types */
typedef struct Vec2f {
    f32 x, y;
} Vec2f;

typedef struct Vec2i {
    i32 x, y;
} Vec2i;

typedef struct Size2i {
    i32 w, h;
} Size2i;

typedef struct Vec3f {
    f32 x, y, z;
} Vec3f;

typedef struct Vec3i {
    i32 x, y, z;
} Vec3i;

typedef struct Rect2f {
    f32 x, y;
    f32 w, h;
} Rect2f;

typedef struct Rect2ui {
    u32 x, y;
    u32 w, h;
} Rect2ui;

typedef struct Color4f {
    f32 r, g, b, a;
} Color4f;

typedef struct Color4i {
    byte8 r, g, b, a;
} Color4i;

typedef struct Mat33f {
    f32 arr[9];
} Mat33f;

typedef struct Mat44f {
    f32 arr[16];
} Mat44f;

static const Mat44f identity4x4 = {.arr = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
static const Mat33f identity3x3 = {.arr = {
                                       1,
                                       0,
                                       0,
                                       0,
                                       1,
                                       0,
                                       0,
                                       0,
                                       1,
                                   }};

#endif /* !__cplusplus */

#define S8_LIT(s) (StringSlice{.data = (char*) &(s)[0], .len = LENGTHOF_LIT(s), .null_term = true})
#define S8_DATA(c, l) (StringSlice{.data = (char*) &(c)[0], .len = (l), .null_term = false})
#define S8_CSTR(s) (StringSlice{.data = (char*) (s), .len = (size_t) strlen(s), .null_term = true})

#define MSTRING_NT(a) (MString{.data = nullptr, .len = 0, .null_term = true, .capacity = 0, .allocator = (a)})
#endif

#ifndef CXB_C_API_DECL
#if !defined(CXB_H) || defined(CXB_C_API)
#define CXB_C_API_DECL

// ** SECTION: StringSlice C functions
CXB_C_EXPORT size_t cxb_ss_size(StringSlice s);
CXB_C_EXPORT size_t cxb_ss_n_bytes(StringSlice s);
CXB_C_EXPORT bool cxb_ss_empty(StringSlice s);
CXB_C_EXPORT const char* cxb_ss_c_str(StringSlice s);
CXB_C_EXPORT StringSlice cxb_ss_slice(StringSlice s, i64 i, i64 j);
CXB_C_EXPORT bool cxb_ss_eq(StringSlice a, StringSlice b);
CXB_C_EXPORT bool cxb_ss_neq(StringSlice a, StringSlice b);
CXB_C_EXPORT bool cxb_ss_lt(StringSlice a, StringSlice b);
CXB_C_EXPORT char cxb_ss_back(StringSlice s);

// ** SECTION: MString C functions
CXB_C_EXPORT size_t cxb_mstring_size(MString s);
CXB_C_EXPORT size_t cxb_mstring_n_bytes(MString s);
CXB_C_EXPORT bool cxb_mstring_empty(MString s);
CXB_C_EXPORT const char* cxb_mstring_c_str(MString s);
CXB_C_EXPORT bool cxb_mstring_eq(MString a, MString b);
CXB_C_EXPORT bool cxb_mstring_neq(MString a, MString b);
CXB_C_EXPORT bool cxb_mstring_lt(MString a, MString b);
CXB_C_EXPORT char cxb_mstring_back(MString s);
CXB_C_EXPORT size_t cxb_mstring_capacity(MString s);

CXB_C_EXPORT void cxb_mstring_destroy(MString* s);
CXB_C_EXPORT void cxb_mstring_ensure_capacity(MString* s, size_t cap);
CXB_C_EXPORT void cxb_mstring_resize(MString* s, size_t size);
CXB_C_EXPORT void cxb_mstring_extend(MString* s, StringSlice slice);
CXB_C_EXPORT void cxb_mstring_push_back(MString* s, char val);
CXB_C_EXPORT char* cxb_mstring_push(MString* s);
CXB_C_EXPORT void cxb_mstring_reserve(MString* s, size_t cap);
CXB_C_EXPORT void cxb_mstring_ensure_null_terminated(MString* s);
CXB_C_EXPORT MString cxb_mstring_copy(MString s, Allocator* to_allocator);
#endif
#endif
