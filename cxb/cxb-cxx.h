/*
# cxb: Base library for CX (Orthodox-C++)

This library supports C for some subset of the library. See `cxb-c.h` for details.

## Memory

* Arena
* Allocator

Both of the above are C-compatible.

## Containers
* C & C++ compatible: use these types when defining a C API
    * `String8`: a pointer to char* (`data`), a length (`len`), and a flag (`not_null_term`) to indicate if the string
is null-terminated
        - `String8` is a POD type, it does not own the memory it points to, it is only a view into a contiguous
        - Arenas are supported with this type, you may append additional characters (if the string is on the arena) and
the `not_null_term` flag will be maintained
    * `MString8`: a manually memory managed string given an allocator, "M" stands for "manual". Call `.destroy()` to
free memory
* C++ only types: use these types when when defining C++ APIs or in implementation files
    * `AString8: an automatically managed string using RAII, compatible with `MString8` and `String8`
        - Destructor calls `destroy()`, i.e. this type is a `std::string` alternative
        - Moves are supported, but unlike std::string, manual copies are required via `copy()`
        - You may call `.release()` to remove ownership of the underlying memory, i.e. such that the destructor does not
free the memory
    * `Array<T>`: TODO
    * `MArray<T>`: TODO
    * `AArray<T>`: TODO

## Math Types
* C and C++ compatible
    * Vec2f
    * Vec2i
    * Size2i
    * Vec3f
    * Vec3i
    * Rect2f
    * Rect2ui
    * Color4f
    * Color4i
    * Mat33f
    * Mat44f
*/

#ifndef CXB_H
#define CXB_H

#ifndef __cplusplus
#error "Include <cxb/cxb-c.h> when compiling C code. <cxb/cxb-cxx.h> is C++-only."
#endif

/* SECTION: configuration */
#if __cpp_concepts
#include <concepts>
#define CXB_USE_CXX_CONCEPTS
#endif

#define CXB_SEQ_MIN_CAP 32
#define CXB_SEQ_GROW_FN(x) (x) + (x) / 2 /* 3/2, reducing chance to overflow */
#define CXB_STR_MIN_CAP 32
#define CXB_STR_GROW_FN(x) (x) + (x) / 2 /* 3/2, reducing chance to overflow  */
#define CXB_HM_MIN_CAP 64
#define CXB_HM_LOAD_CAP_THRESHOLD 0.75

// NOTE: to generate cxb-c.h (C header)
#define CXB_C_COMPAT_BEGIN
#define CXB_C_COMPAT_END
#define CXB_C_TYPE

/* SECTION: includes */
CXB_C_COMPAT_BEGIN
#include <stdatomic.h> // _Atomic(T)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // atof
#include <string.h>
CXB_C_COMPAT_END

#include <initializer_list>
#include <limits>
#include <new>
#include <type_traits> // 27ms
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#ifdef __cplusplus
#define CXB_C_EXPORT extern "C"
#define CXB_C_IMPORT extern "C"
#define C_DECL_BEGIN extern "C" {
#define C_DECL_END }
#else
#define CXB_C_EXPORT
#define CXB_C_IMPORT
#define C_DECL_BEGIN
#define C_DECL_END
#endif

/* SECTION: macros */
// NOTE some macros are copied/modified from Blend2D, see:
// https://github.com/blend2d/blend2d/blob/bae3b6c600186a69a9f212243ed9700dc93a314a/src/blend2d/api.h#L563
CXB_C_COMPAT_BEGIN
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

// NOTE: these are C++ only, as they use a C++ formatting library
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
CXB_C_COMPAT_END

#if defined(__clang_major__) && __clang_major__ >= 6
#define CXB_PURE constexpr __attribute__((__pure__))
#elif defined(__GNUC__) && __GNUC__ >= 6
#define CXB_PURE constexpr __attribute__((__pure__))
#else
#define CXB_PURE constexpr
#endif

template <typename T>
static inline const T& min(const T& a, const T& b) {
    return a < b ? a : b;
}

template <typename T>
static inline const T& max(const T& a, const T& b) {
    return a > b ? a : b;
}

/* NOTE: #include <utility>  // takes 98ms */
template <typename T>
static inline typename std::remove_reference<T>::type&& move(T&& v) noexcept {
    return static_cast<typename std::remove_reference<T>::type&&>(v);
}

template <typename T>
static inline T&& forward(typename std::remove_reference<T>::type& v) noexcept {
    return static_cast<T&&>(v);
}

template <typename T>
static inline T&& forward(typename std::remove_reference<T>::type&& v) noexcept {
    return static_cast<T&&>(v);
}

/* SECTION: primitive functions */
template <typename T>
CXB_MAYBE_INLINE void construct(T* xs, size_t len) {
    ASSERT(len > 0);
    if constexpr(!std::is_trivially_default_constructible_v<T>) {
        for(size_t i = 0; i < len; ++i) {
            new(xs + i) T{};
        }
    } else {
        memset(xs, 0, sizeof(T) * len);
    }
}

template <typename T>
CXB_MAYBE_INLINE void construct(T* xs, size_t len, T default_value) {
    ASSERT(len > 0);
    if constexpr(!std::is_trivially_default_constructible_v<T>) {
        for(size_t i = 0; i < len; ++i) {
            new(xs + i) T{default_value};
        }
    } else {
        for(size_t i = 0; i < len; ++i) {
            memcpy((void*) (xs + i), (void*) (&default_value), sizeof(T));
        }
    }
}

template <typename T, typename... Args>
CXB_MAYBE_INLINE void construct(T* xs, size_t len, Args&&... args) {
    ASSERT(len > 0);
    if constexpr(!std::is_trivially_default_constructible_v<T>) {
        for(size_t i = 0; i < len; ++i) {
            new(xs + i) T{args...};
        }
    } else {
        // TODO: optimize?
        for(size_t i = 0; i < len; ++i) {
            new(xs + i) T{args...};
        }
    }
}

template <typename T>
CXB_MAYBE_INLINE void destroy(T* xs, size_t len) {
    if constexpr(!std::is_trivially_destructible_v<T>) {
        for(size_t i = 0; i < len; ++i) {
            xs[i].~T();
        }
    }
}

template <typename T>
CXB_MAYBE_INLINE void copy(T* dst, const T* src, size_t n) {
    if constexpr(std::is_trivially_assignable_v<T, T>) {
        memcpy(dst, src, n * sizeof(T));
    } else {
        for(size_t i = 0; i < n; ++i) {
            dst[i] = src[i];
        }
    }
}

template <typename T>
CXB_PURE const T& clamp(const T& x, const T& a, const T& b) {
    return a < b ? max(min(b, x), a) : min(max(a, x), b);
}

template <typename T>
static inline void swap(T& t1, T& t2) noexcept {
    T temp(move(t1));
    t1 = move(t2);
    t2 = move(temp);
}

static CXB_INLINE u64 pow2mod(u64 x, u64 b) {
    DEBUG_ASSERT(b != 0 && (b & (b - 1)) == 0, "{} is not a power of 2", b);
    return x & (b - 1);
}

// ref: https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static CXB_INLINE u64 round_up_pow2(u64 x) {
    if(x <= 1) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
#if UINTPTR_MAX > 0xFFFFFFFFu
    x |= x >> 32;
#endif
    x++;
    return x;
}

/* SECTION: arena */
struct Arena;
struct String8;
struct String8SplitIterator;
template <typename T>
struct Array;
template <typename T, size_t N>
struct StaticArray;
struct Allocator;

#ifdef CXB_USE_CXX_CONCEPTS
template <typename A>
concept ArrayLike = requires(A x) {
    { x.data } -> std::convertible_to<void*>;
    { x.len } -> std::convertible_to<u64>;
};
#endif

// TODO: forward decl
String8 arena_push_string8(Arena* arena, size_t n = 1);
String8 arena_push_string8(Arena* arena, String8 to_copy);
void string8_resize(String8& str, Arena* arena, size_t n, char fill_char = '\0');
void string8_push_back(String8& str, Arena* arena, char ch);
void string8_pop_back(String8& str, Arena* arena);
void string8_pop_all(String8& str, Arena* arena);
void string8_insert(String8& str, Arena* arena, char ch, size_t i);
void string8_insert(String8& str, Arena* arena, String8 to_insert, size_t i);
void string8_extend(String8& str, Arena* arena, String8 to_append);
CXB_PURE bool string8_contains(const String8& s, String8 needle);
CXB_PURE bool string8_contains_chars(const String8& s, String8 chars);
CXB_PURE size_t string8_find(const String8& s, String8 needle);
CXB_PURE String8 string8_trim(const String8& s, String8 chars, bool leading = true, bool trailing = true);
CXB_PURE String8 string8_trim_all(const String8& s, String8 chars, bool leading = true, bool trailing = true);
CXB_PURE bool string8_starts_with(const String8& s, String8 prefix);
CXB_PURE bool string8_ends_with(const String8& s, String8 suffix);

template <typename T>
Array<T> arena_push_array(Arena* arena, size_t n);
template <typename T>
Array<T> arena_push_array(Arena* arena, Array<T> to_copy);

CXB_C_TYPE struct ArenaParams {
    CXB_C_COMPAT_BEGIN
    size_t reserve_bytes;
    size_t max_n_blocks;
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Arena {
    CXB_C_COMPAT_BEGIN
    char* start;
    char* end;
    size_t pos;
    ArenaParams params;

    // chaining
    Arena* next;
    Arena* prev;
    size_t n_blocks;

    // TODO: freelist
    CXB_C_COMPAT_END

    CXB_MAYBE_INLINE Allocator make_alloc();
    CXB_MAYBE_INLINE Allocator* push_alloc();
};

CXB_C_EXPORT Arena* arena_make(ArenaParams params);
CXB_C_EXPORT Arena* arena_make_nbytes(size_t n_bytes);
CXB_C_EXPORT void arena_destroy(Arena* arena);

CXB_C_EXPORT void* arena_push_bytes(Arena* arena, size_t size, size_t align);
CXB_C_EXPORT void arena_pop_to(Arena* arena, u64 pos);
CXB_C_EXPORT void arena_clear(Arena* arena);

CXB_C_TYPE struct ArenaTmp {
    CXB_C_COMPAT_BEGIN
    Arena* arena;
    u64 pos;
    CXB_C_COMPAT_END
};

Arena* get_perm();
ArenaTmp begin_scratch();
void end_scratch(const ArenaTmp& tmp);

struct AArenaTmp : ArenaTmp {
    AArenaTmp(const ArenaTmp& other) : ArenaTmp{other} {}
    CXB_INLINE ~AArenaTmp() {
        end_scratch(*this);
    }
};

// *SECTION: Arena free functions
template <typename T>
inline T* arena_push_fast(Arena* arena, size_t n = 1) {
    T* data = (T*) arena_push_bytes(arena, sizeof(T) * n, alignof(T));
    return data;
}

template <typename T>
inline T* arena_push(Arena* arena, size_t n = 1) {
    T* data = (T*) arena_push_bytes(arena, sizeof(T) * n, alignof(T));
    ::construct(data, n);
    return data;
}

template <typename T>
inline T* arena_push(Arena* arena, size_t n, T default_value) {
    T* data = (T*) arena_push_bytes(arena, sizeof(T) * n, alignof(T));
    ::construct(data, n, default_value);
    return data;
}

template <typename T>
inline void arena_pop(Arena* arena, T* x) {
    ASSERT((void*) (x) >= (void*) arena->start && (void*) (x) < arena->end, "array not allocated on arena");
    ASSERT((void*) (x + 1) == (void*) (arena->start + arena->pos), "cannot pop unless array is at the end");
    arena->pos -= sizeof(T);
}

// *SECTION: Array<T> functions
template <typename T>
inline Array<T> arena_push_array(Arena* arena, size_t n) {
    T* data = arena_push<T>(arena, n);
    return Array<T>{data, n};
}

template <typename T>
inline Array<T> arena_push_array(Arena* arena, Array<T> to_copy) {
    T* data = arena_push<T>(arena, to_copy.len);
    ::copy(data, to_copy.data, to_copy.len);
    return Array<T>{.data = data, .len = to_copy.len};
}

template <typename A>
inline void array_resize_fast(A& xs, Arena* arena, size_t new_size)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A>
#endif
{
    using T = std::remove_reference_t<decltype(xs.data[0])>;

    if(UNLIKELY(new_size == xs.len)) return;

    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) ||
                     ((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end),
                 "array not allocated on arena");
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) || (void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos),
                 "cannot resize unless array is at the end");
    if(new_size > xs.len) {
        T* data = arena_push_fast<T>(arena, new_size - xs.len);
        xs.data = xs.data == nullptr ? data : xs.data;
        xs.len = new_size;
    } else {
        arena_pop_to(arena, arena->pos + (sizeof(T) * (xs.len - new_size)));
        xs.len = new_size;
    }
}

template <typename A>
inline void array_resize(A& xs, Arena* arena, size_t new_size)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A>
#endif
{
    using T = std::remove_reference_t<decltype(xs.data[0])>;

    if(UNLIKELY(new_size == xs.len)) return;

    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) ||
                     ((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end),
                 "array not allocated on arena");
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) || (void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos),
                 "cannot resize unless array is at the end");
    if(new_size > xs.len) {
        T* data = arena_push<T>(arena, new_size - xs.len);
        xs.data = xs.data == nullptr ? data : xs.data;
        xs.len = new_size;
    } else {
        arena_pop_to(arena, arena->pos - (sizeof(T) * (xs.len - new_size)));
        xs.len = new_size;
    }
}

template <typename A, typename T>
inline void array_resize(A& xs, Arena* arena, size_t new_size, T default_value)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A>
#endif
{
#ifndef CXB_USE_CXX_CONCEPTS
    using T = std::remove_reference_t<decltype(xs.data[0])>;
#endif

    if(UNLIKELY(new_size == xs.len)) return;

    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) ||
                     ((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end),
                 "array not allocated on arena");
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) || ((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos)),
                 "cannot resize unless array is at the end");
    if(new_size > xs.len) {
        T* data = arena_push<T>(arena, new_size - xs.len, default_value);
        xs.data = xs.data == nullptr ? data : xs.data;
        xs.len = new_size;
    } else {
        arena_pop_to(arena, arena->pos + (sizeof(T) * (xs.len - new_size)));
        xs.len = new_size;
    }
}

template <typename A, typename T>
inline void array_push_back(A& xs, Arena* arena, T value)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A>
#endif
{
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) ||
                     ((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end),
                 "array not allocated on arena");
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) || (void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos),
                 "cannot push unless array is at the end");
    void* data = arena_push_bytes(arena, sizeof(T), alignof(T));
    xs.data = UNLIKELY(xs.data == nullptr) ? (T*) data : xs.data;
    xs.data[xs.len] = value;
    xs.len += 1;
}

template <typename A, typename... Args>
inline void array_emplace_back(A& xs, Arena* arena, Args&&... args)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A>
#endif
{
    using T = std::remove_reference_t<decltype(xs.data[0])>;

    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) ||
                     ((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end),
                 "array not allocated on arena");
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) || ((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos)),
                 "cannot push unless array is at the end");

    void* data = arena_push_bytes(arena, sizeof(T), alignof(T));
    xs.data = UNLIKELY(xs.data == nullptr) ? (T*) data : xs.data;
    xs.data[xs.len] = T{forward<Args>(args)...};
    xs.len += 1;
}

template <typename A>
inline void array_pop_back(A& xs, Arena* arena)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A>
#endif
{
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) ||
                     ((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end),
                 "array not allocated on arena");
    DEBUG_ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos),
                 "cannot pop unless array is at the end");
    ::destroy(&xs.data[xs.len - 1], 1);
    arena_pop_to(arena, arena->pos - sizeof(xs.data[0]));
    xs.len -= 1;
}

template <typename A, typename B>
inline void array_insert(A& xs, Arena* arena, const B& to_insert, size_t i)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A> && ArrayLike<B>
#endif
{
    using T = std::remove_reference_t<decltype(xs.data[0])>;

    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) ||
                     ((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end),
                 "array not allocated on arena");
    DEBUG_ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos),
                 "cannot push unless array is at the end");
    DEBUG_ASSERT(i <= xs.len, "insert position out of bounds");

    arena_push_bytes(arena, to_insert.len * sizeof(T), alignof(T));

    size_t old_len = xs.len;
    xs.len += to_insert.len;

    memmove(xs.data + i + to_insert.len, xs.data + i, (old_len - i) * sizeof(T));
    ::copy(xs.data + i, to_insert.data, to_insert.len);
}

template <typename A, typename B>
inline void array_extend(A& xs, Arena* arena, const B& to_append)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A> && ArrayLike<B>
#endif
{
    using T = std::remove_reference_t<decltype(xs.data[0])>;

    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) ||
                     ((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end),
                 "array not allocated on arena");
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) || (void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos),
                 "cannot extend unless array is at the end");

    void* data = arena_push_bytes(arena, to_append.len * sizeof(T), alignof(T));
    xs.data = UNLIKELY(xs.data == nullptr) ? (T*) data : xs.data;

    size_t old_len = xs.len;
    xs.len += to_append.len;

    ::copy(xs.data + old_len, to_append.data, to_append.len);
}

template <typename A>
inline void array_pop_all(A& xs, Arena* arena)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A>
#endif
{
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) ||
                     ((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end),
                 "array not allocated on arena");
    DEBUG_ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos),
                 "cannot pop unless array is at the end");
    arena_pop_to(arena, arena->pos - (sizeof(xs.data[0]) * xs.len));
    xs.data = nullptr;
    xs.len = 0;
}

/* SECTION: algorithms */
struct LessThan {
    template <typename T>
    bool operator()(const T& a, const T& b) const {
        return a < b;
    }
};

template <typename T, typename Compare>
static void merge_sort_impl(T* data, T* tmp, u64 left, u64 right, const Compare& cmp) {
    if(right - left <= 1) return;

    u64 mid = left + ((right - left) >> 1);
    merge_sort_impl(data, tmp, left, mid, cmp);
    merge_sort_impl(data, tmp, mid, right, cmp);

    u64 i = left;
    u64 j = mid;
    u64 k = 0;
    while(i < mid && j < right) {
        if(cmp(data[j], data[i])) {
            tmp[k++] = ::move(data[j++]);
        } else {
            tmp[k++] = ::move(data[i++]);
        }
    }
    while(i < mid) tmp[k++] = ::move(data[i++]);
    while(j < right) tmp[k++] = ::move(data[j++]);
    for(u64 t = 0; t < k; ++t) {
        data[left + t] = ::move(tmp[t]);
    }
}

template <typename T, typename Compare = LessThan>
inline void merge_sort(T* data, u64 len, const Compare& cmp = Compare{}) {
    if(len <= 1) return;
    AArenaTmp scratch = begin_scratch();
    T* tmp = arena_push_fast<T>(scratch.arena, len);
    merge_sort_impl(data, tmp, 0, len, cmp);
}

/* SECTION: general allocation */
CXB_C_TYPE struct Allocator {
    CXB_C_COMPAT_BEGIN
    void* (*alloc_proc)(void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes, bool fill_zeros, void* data);
    void (*free_proc)(void* head, size_t n_bytes, void* data);
    void (*free_all_proc)(void* data);
    void* data;
    CXB_C_COMPAT_END

    template <typename T, typename H>
    struct AllocationWithHeader {
        T* data;
        H* header;
    };

    void free_all() {
        this->free_all_proc(data);
    }

    template <typename T>
    CXB_MAYBE_INLINE T* alloc(size_t count = 1) {
        T* result = (T*) this->alloc_proc(nullptr, sizeof(T) * count, alignof(T), 0, false, data);
        return result;
    }

    template <typename T>
    CXB_MAYBE_INLINE T* calloc(size_t old_count, size_t count = 1) {
        T* result = (T*) this->alloc_proc(nullptr, sizeof(T) * count, alignof(T), sizeof(T) * old_count, true, data);
        return result;
    }

    template <typename T>
    CXB_MAYBE_INLINE T* realloc(T* head, size_t old_count, bool fill_zeros, size_t count) {
        T* result =
            (T*) this->alloc_proc((void*) head, sizeof(T) * count, alignof(T), sizeof(T) * old_count, fill_zeros, data);
        return result;
    }

    template <typename H, typename T>
    CXB_MAYBE_INLINE AllocationWithHeader<T, H> realloc_with_header(H* header, size_t old_count, size_t count) {
        char* new_header = (char*) this->alloc_proc((void*) header,                    // header
                                                    sizeof(T) * count + sizeof(H),     // n_bytes
                                                    alignof(T) + sizeof(H),            // alignment
                                                    sizeof(T) * old_count + sizeof(H), // old bytes
                                                    false,                             // fill_zeros
                                                    this->data);
        T* data = (T*) (new_header + sizeof(H));
        return AllocationWithHeader<T, H>{data, (H*) new_header};
    }

    template <typename H, typename T>
    CXB_MAYBE_INLINE AllocationWithHeader<T, H> recalloc_with_header(H* header, size_t old_count, size_t count) {
        char* new_header = (char*) this->alloc_proc((void*) header,                                      // header
                                                    sizeof(T) * count + sizeof(H),                       // n_bytes
                                                    alignof(T) + sizeof(H),                              // alignment
                                                    sizeof(T) * old_count + sizeof(H) * (old_count > 0), // old bytes
                                                    true,                                                // fill_zeros
                                                    this->data);
        T* data = (T*) (new_header + sizeof(H));
        return AllocationWithHeader<T, H>{data, (H*) new_header};
    }

    template <typename H, typename T>
    CXB_MAYBE_INLINE void free_header_offset(T* offset_from_header, size_t count) {
        this->free_proc((char*) (offset_from_header) - sizeof(H), sizeof(T) * count + sizeof(H), this->data);
    }

    template <typename T>
    CXB_MAYBE_INLINE void free(T* head, size_t count) {
        this->free_proc((void*) head, sizeof(T) * count, this->data);
    }
};

Allocator make_arena_alloc(Arena* arena);
Allocator* push_arena_alloc(Arena* arena);
CXB_MAYBE_INLINE Allocator Arena::make_alloc() {
    return make_arena_alloc(this);
}
CXB_MAYBE_INLINE Allocator* Arena::push_alloc() {
    return push_arena_alloc(this);
}

CXB_C_COMPAT_BEGIN
extern Allocator heap_alloc;
CXB_C_COMPAT_END

struct ThreadLocalRuntime {
    Arena* perm;
    Arena* scratch[2];
    int scratch_idx;
};

struct CxbRuntimeParams {
    ArenaParams perm_params;
    ArenaParams scratch_params;
};

extern thread_local ThreadLocalRuntime cxb_runtime;
void cxb_init(CxbRuntimeParams);

/* SECTION: variant types (1/2) */
template <typename T, typename EC>
struct Result;

template <typename T>
struct Optional;

template <typename T>
struct ParseResult;

/* SECTION: basic types */
CXB_C_TYPE struct String8 {
    CXB_C_COMPAT_BEGIN
    char* data;
    union {
        struct {
            size_t len : 63;
            bool not_null_term : 1;
        };
        size_t metadata;
    };
    CXB_C_COMPAT_END

    // ** SECTION: slice compatible methods
    CXB_MAYBE_INLINE size_t n_bytes() const {
        return len + !not_null_term;
    }
    CXB_MAYBE_INLINE size_t size() const {
        return len;
    }
    CXB_MAYBE_INLINE bool empty() const {
        return len == 0;
    }
    CXB_MAYBE_INLINE char& operator[](size_t idx) {
        return data[idx];
    }
    CXB_MAYBE_INLINE const char& operator[](size_t idx) const {
        return data[idx];
    }
    CXB_MAYBE_INLINE char& back() {
        return data[len - 1];
    }
    CXB_MAYBE_INLINE String8 slice(i64 i = 0, i64 j = -1) const {
        if(!data) {
            return {};
        }
        i64 ii = clamp(i < 0 ? (i64) len + i : i, (i64) 0, len == 0 ? 0 : (i64) len - 1);
        i64 jj = clamp(j < 0 ? (i64) len + j : j, (i64) 0, len == 0 ? 0 : (i64) len - 1);

        String8 c = *this;
        c.data = c.data + ii;
        c.len = max<i64>(0, jj - ii + 1);
        c.not_null_term = ii + c.len == len ? this->not_null_term : true;
        return c;
    }

    CXB_MAYBE_INLINE const char* c_str() const {
        return not_null_term ? nullptr : data;
    }

    CXB_MAYBE_INLINE const char* c_str_maybe_copy(Arena* a) const {
        if(not_null_term) {
            char* new_data = arena_push_fast<char>(a, len + 1);
            ::copy(new_data, data, len);
            new_data[len] = '\0';
            return new_data;
        }
        return data;
    }

    CXB_MAYBE_INLINE int compare(const String8& o) const {
        int result = memcmp(data, o.data, len < o.len ? len : o.len);
        if(result == 0) {
            return len - o.len;
        }
        return result;
    }

    CXB_MAYBE_INLINE bool operator==(const String8& o) const {
        return compare(o) == 0;
    }

    CXB_MAYBE_INLINE bool operator!=(const String8& o) const {
        return !(*this == o);
    }

    CXB_MAYBE_INLINE bool operator<(const String8& o) const {
        size_t n = len < o.len ? len : o.len;
        int cmp = memcmp(data, o.data, n);
        if(cmp < 0) return true;
        if(cmp > 0) return false;
        return len < o.len;
    }

    CXB_MAYBE_INLINE bool operator>(const String8& o) const {
        return o < *this;
    }

    CXB_INLINE bool contains(String8 needle) const {
        return string8_contains(*this, needle);
    }
    CXB_INLINE bool contains_chars(String8 chars) const {
        return string8_contains_chars(*this, chars);
    }
    CXB_INLINE size_t find(String8 needle) const {
        return string8_find(*this, needle);
    }
    CXB_INLINE bool starts_with(String8 prefix) const {
        return string8_starts_with(*this, prefix);
    }
    CXB_INLINE bool ends_with(String8 suffix) const {
        return string8_ends_with(*this, suffix);
    }
    CXB_INLINE String8 trim(String8 chars, bool leading = true, bool trailing = true) const {
        return string8_trim(*this, chars, leading, trailing);
    }
    CXB_INLINE String8 trim_all(String8 chars, bool leading = true, bool trailing = true) const {
        return string8_trim_all(*this, chars, leading, trailing);
    }
    CXB_INLINE String8 trim_left(String8 chars) const {
        return string8_trim(*this, chars, true, false);
    }
    CXB_INLINE String8 trim_right(String8 chars) const {
        return string8_trim(*this, chars, false, true);
    }
    CXB_INLINE String8 trim_all_left(String8 chars) const {
        return string8_trim_all(*this, chars, true, false);
    }
    CXB_INLINE String8 trim_all_right(String8 chars) const {
        return string8_trim_all(*this, chars, false, true);
    }

    // ** SECTION: arena UFCS
    CXB_INLINE void resize(Arena* arena, size_t n, char fill_char = '\0') {
        string8_resize(*this, arena, n, fill_char);
    }
    CXB_INLINE void push_back(Arena* arena, char ch) {
        string8_push_back(*this, arena, ch);
    }
    CXB_INLINE void pop_back(Arena* arena) {
        string8_pop_back(*this, arena);
    }
    CXB_INLINE void pop_all(Arena* arena) {
        string8_pop_all(*this, arena);
    }
    CXB_INLINE void insert(Arena* arena, char ch, size_t i) {
        string8_insert(*this, arena, ch, i);
    }
    CXB_INLINE void insert(Arena* arena, String8 to_insert, size_t i) {
        string8_insert(*this, arena, to_insert, i);
    }
    CXB_INLINE void extend(Arena* arena, String8 to_append) {
        string8_extend(*this, arena, to_append);
    }

    CXB_INLINE String8SplitIterator split(String8 delim) const;
    CXB_INLINE String8SplitIterator split_any(String8 chars) const;

    // *SECTION*: parsing
    template <typename T>
    CXB_INLINE std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, ParseResult<T>> parse(
        u64 base = 10) const {
        return string8_parse<T>(*this, base);
    }
};

/* SECTION: variant types (2/2) */
template <typename T>
struct Optional {
    T value;
    bool exists;

    inline operator bool() const {
        return exists;
    }
};

template <typename T, typename EC>
struct Result {
    T value;
    EC error;
    String8 reason;

    inline operator bool() const {
        return (i64) error != 0;
    }
};

// TODO: Result<T> ?
template <typename T>
struct ParseResult {
    T value;
    bool exists;
    size_t n_consumed;

    inline operator bool() const {
        return exists;
    }
};

template <typename T>
CXB_MAYBE_INLINE std::enable_if_t<std::is_integral_v<T>, ParseResult<T>> string8_parse(String8 str, u64 base = 10) {
    ASSERT(base <= 10, "TODO: support bases > 10");
    ParseResult<T> result = {.value = {}, .exists = str.len > 0, .n_consumed = 0};
    u64 num_negs = 0;
    for(u64 i = 0; i < str.len; ++i) {
        if(str[i] == '-') {
            num_negs += 1;
            result.value *= -1;
            result.n_consumed += 1;
            continue;
            // TODO: support other bases
        } else if(str[i] >= '0' && str[i] <= '9') {
            result.value *= base;
            result.value += str[i] - '0';
            result.n_consumed += 1;
        } else {
            break;
        }
    }
    if constexpr(std::is_unsigned_v<T>) {
        if(num_negs > 0) result.exists = false;
    } else {
        if(num_negs > 1) result.exists = false;
    }
    result.exists = result.n_consumed > 0;
    return result;
}

template <typename T>
CXB_MAYBE_INLINE std::enable_if_t<std::is_floating_point_v<T>, ParseResult<T>> string8_parse(String8 str,
                                                                                             u64 base = 10) {
    ASSERT(base == 10, "only base 10 supported for floats");
    ParseResult<T> result = {.value = {}, .exists = str.len > 0, .n_consumed = 0};

    ArenaTmp tmp = begin_scratch();
    result.value = atof(str.c_str_maybe_copy(tmp.arena));
    result.n_consumed = str.len; // TODO
    result.exists = true;
    end_scratch(tmp);
    return result;
}

template <typename T>
struct Array {
    T* data;
    size_t len;

    Array() : data{nullptr}, len{0} {}
    Array(T* data, size_t len) : data{data}, len{len} {}
    Array(std::initializer_list<T>) = delete;
    Array(Arena* a, std::initializer_list<T> xs) : data{nullptr}, len{0} {
        data = arena_push_fast<T>(a, xs.size());
        len = xs.size();
        ::copy(data, xs.begin(), xs.size());
    }
    Array(const Array<T>& o) : data{o.data}, len{o.len} {}
    Array<T>& operator=(const Array<T>& o) {
        data = o.data;
        len = o.len;
        return *this;
    }
    Array(Array<T>&& o) : data{o.data}, len{o.len} {
        o.data = nullptr;
        o.len = 0;
    }
    Array<T>& operator=(Array<T>&& o) {
        data = o.data;
        len = o.len;
        o.data = nullptr;
        o.len = 0;
        return *this;
    }
    ~Array() = default;

    CXB_MAYBE_INLINE size_t size() const {
        return len;
    }
    CXB_MAYBE_INLINE bool empty() const {
        return len == 0;
    }
    CXB_MAYBE_INLINE T& operator[](size_t idx) {
        DEBUG_ASSERT(idx < len, "index out of bounds {} >= {}", idx, len);
        return data[idx];
    }
    CXB_MAYBE_INLINE const T& operator[](size_t idx) const {
        DEBUG_ASSERT(idx < len, "index out of bounds {} >= {}", idx, len);
        return data[idx];
    }
    CXB_MAYBE_INLINE T& back() {
        return data[len - 1];
    }
    CXB_MAYBE_INLINE Array<T> slice(i64 i = 0, i64 j = -1) {
        i64 ii = clamp(i < 0 ? (i64) len + i : i, (i64) 0, (i64) len - 1);
        i64 jj = clamp(j < 0 ? (i64) len + j : j, (i64) 0, (i64) len - 1);
        DEBUG_ASSERT(ii <= jj, "i > j: {} ({}) < {} ({})", ii, i, jj, j);

        Array<T> c = *this;
        c.data = c.data + ii;
        c.len = jj - ii + 1;
        return c;
    }

    CXB_MAYBE_INLINE bool operator<(const Array<T>& o) const {
        size_t n = len < o.len ? len : o.len;
        for(size_t i = 0; i < n; ++i) {
            if(o.data[i] < data[i])
                return false;
            else if(data[i] < o.data[i])
                return true;
        }
        return len < o.len;
    }

    CXB_MAYBE_INLINE bool operator==(const Array<T>& o) const {
        if(len != o.len) return false;
        for(size_t i = 0; i < len; ++i) {
            if(!(data[i] == o.data[i])) return false;
        }
        return true;
    }

    CXB_MAYBE_INLINE bool operator!=(const Array<T>& o) const {
        return !(*this == o);
    }

    CXB_MAYBE_INLINE bool operator>(const Array<T>& o) const {
        return o < *this;
    }

    CXB_MAYBE_INLINE T* begin() {
        return data;
    }
    CXB_MAYBE_INLINE T* end() {
        return data + len;
    }
    CXB_MAYBE_INLINE const T* begin() const {
        return data;
    }
    CXB_MAYBE_INLINE const T* end() const {
        return data + len;
    }

    // ** SECTION: arena UFCS
    CXB_INLINE void resize_fast(Arena* arena, size_t n) {
        array_resize_fast(*this, arena, n);
    }
    CXB_INLINE void resize(Arena* arena, size_t n) {
        array_resize(*this, arena, n);
    }
    CXB_INLINE void resize(Arena* arena, size_t n, T value) {
        array_resize(*this, arena, n, value);
    }
    CXB_INLINE void push_back(Arena* arena, T x) {
        array_push_back(*this, arena, x);
    }
    CXB_INLINE void pop_back(Arena* arena) {
        array_pop_back(*this, arena);
    }
    CXB_INLINE void pop_all(Arena* arena) {
        array_pop_all(*this, arena);
    }
    CXB_INLINE void insert(Arena* arena, T value, size_t i) {
        StaticArray<T, 1> tmp{{value}};
        array_insert(*this, arena, tmp, i);
    }
    CXB_INLINE void insert(Arena* arena, Array<T> to_insert, size_t i) {
        array_insert(*this, arena, to_insert, i);
    }
    template <size_t N>
    CXB_INLINE void insert(Arena* arena, const StaticArray<T, N>& to_insert, size_t i) {
        array_insert(*this, arena, to_insert, i);
    }
    CXB_INLINE void extend(Arena* arena, Array<T> to_append) {
        array_extend(*this, arena, to_append);
    }
    template <size_t N>
    CXB_INLINE void extend(Arena* arena, const StaticArray<T, N>& to_append) {
        array_extend(*this, arena, to_append);
    }

    template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, char>>>
    CXB_MAYBE_INLINE String8 as_string8(bool nt_len = true) const {
        bool is_nt = (len > 0 && data[len - 1] == '\0');
        return String8{.data = data, .len = len - (is_nt && nt_len), .not_null_term = !is_nt};
    }
};

// *SECTION*: String8 splitting

CXB_C_EXPORT bool string8_split_next(String8SplitIterator* iter, String8* out);
Array<String8> string8_split_collect(Arena* arena, String8SplitIterator iter);

CXB_C_TYPE struct String8SplitIterator {
    CXB_C_COMPAT_BEGIN
    String8 s;
    String8 delim;
    u64 pos;
    String8 curr;
    bool any;
    CXB_C_COMPAT_END

#ifdef __cplusplus
    CXB_INLINE bool next(String8& out) {
        bool has = string8_split_next(this, &out);
        if(has) curr = out;
        return has;
    }
    CXB_INLINE String8SplitIterator begin() {
        string8_split_next(this, &curr);
        return *this;
    }
    CXB_INLINE String8SplitIterator end() const {
        return String8SplitIterator{.s = s, .delim = delim, .pos = s.len + 1, .curr = {}, .any = any};
    }
    CXB_INLINE bool operator!=(const String8SplitIterator& o) const {
        return curr.data != o.curr.data;
    }
    CXB_INLINE String8 operator*() const {
        return curr;
    }
    CXB_INLINE String8SplitIterator& operator++() {
        if(!string8_split_next(this, &curr)) {
            curr = {};
            pos = s.len + 1;
        }
        return *this;
    }
    CXB_INLINE Array<String8> collect(Arena* arena) {
        return string8_split_collect(arena, *this);
    }
#endif // __cplusplus
};

CXB_INLINE String8SplitIterator string8_split(String8 s, String8 delim) {
    ASSERT(UNLIKELY(delim.len != 0), "delimiter is an empty string");
    return String8SplitIterator{.s = s, .delim = delim, .pos = 0, .curr = {}, .any = false};
}

CXB_INLINE String8SplitIterator string8_split_any(String8 s, String8 chars) {
    ASSERT(UNLIKELY(chars.len != 0), "delimiter is an empty string");
    return String8SplitIterator{.s = s, .delim = chars, .pos = 0, .curr = {}, .any = true};
}

CXB_INLINE Array<String8> string8_split_collect(Arena* arena, String8SplitIterator iter) {
    Array<String8> out{};
    String8 part;
    while(string8_split_next(&iter, &part)) {
        array_push_back(out, arena, part);
    }
    return out;
}

CXB_INLINE String8SplitIterator String8::split(String8 delim) const {
    return string8_split(*this, delim);
}

CXB_INLINE String8SplitIterator String8::split_any(String8 chars) const {
    return string8_split_any(*this, chars);
}

// *SECTION*: formatting library
template <typename T, size_t N>
struct StaticArray {
    T data[N];
    size_t len = N;

    CXB_INLINE operator Array<T>() & {
        return Array<T>{data, len};
    }
    CXB_INLINE operator Array<T>() && = delete;
};

template <typename T, size_t N>
CXB_INLINE StaticArray<T, N> make_static_array(const T (&xs)[N]) {
    StaticArray<T, N> sa{};
    ::copy(sa.data, xs, N);
    return sa;
}

CXB_C_TYPE struct Vec2f {
    CXB_C_COMPAT_BEGIN
    f32 x, y;
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Vec2i {
    CXB_C_COMPAT_BEGIN
    i32 x, y;
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Size2i {
    CXB_C_COMPAT_BEGIN
    i32 w, h;
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Vec3f {
    CXB_C_COMPAT_BEGIN
    f32 x, y, z;
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Vec3i {
    CXB_C_COMPAT_BEGIN
    i32 x, y, z;
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Rect2f {
    CXB_C_COMPAT_BEGIN
    f32 x, y;
    f32 w, h;
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Rect2ui {
    CXB_C_COMPAT_BEGIN
    u32 x, y;
    u32 w, h;
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Color4f {
    CXB_C_COMPAT_BEGIN
    f32 r, g, b, a;
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Color4i {
    CXB_C_COMPAT_BEGIN
    byte8 r, g, b, a;
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Mat33f {
    CXB_C_COMPAT_BEGIN
    f32 arr[9];
    CXB_C_COMPAT_END
};

CXB_C_TYPE struct Mat44f {
    CXB_C_COMPAT_BEGIN
    f32 arr[16];
    CXB_C_COMPAT_END
};

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

CXB_C_TYPE struct MString8 {
    CXB_C_COMPAT_BEGIN
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
    CXB_C_COMPAT_END

    // ** SECTION: slice compatible methods - delegate to String8
    CXB_MAYBE_INLINE size_t n_bytes() const {
        return len + not_null_term;
    }
    CXB_MAYBE_INLINE size_t size() const {
        return len;
    }
    CXB_MAYBE_INLINE bool empty() const {
        return len == 0;
    }
    CXB_MAYBE_INLINE char& operator[](size_t idx) {
        DEBUG_ASSERT(idx < len, "out of bounds: {} >= {}", idx, len);
        return data[idx];
    }
    CXB_MAYBE_INLINE const char& operator[](size_t idx) const {
        DEBUG_ASSERT(idx < len, "out of bounds: {} >= {}", idx, len);
        return data[idx];
    }
    CXB_MAYBE_INLINE char& back() {
        DEBUG_ASSERT(len > 0, "empty string");
        return data[len - 1];
    }
    CXB_MAYBE_INLINE operator String8() const {
        return *reinterpret_cast<const String8*>(this);
    }

    CXB_MAYBE_INLINE String8 slice(i64 i = 0, i64 j = -1) const {
        return reinterpret_cast<const String8*>(this)->slice(i, j);
    }

    CXB_MAYBE_INLINE const char* c_str() const {
        return not_null_term ? nullptr : data;
    }

    CXB_MAYBE_INLINE bool operator==(const String8& o) const {
        return reinterpret_cast<const String8*>(this)->operator==(o);
    }

    CXB_MAYBE_INLINE bool operator!=(const String8& o) const {
        return !(*this == o);
    }

    CXB_MAYBE_INLINE int compare(const String8& o) const {
        return reinterpret_cast<const String8*>(this)->compare(o);
    }

    CXB_MAYBE_INLINE bool operator<(const String8& o) const {
        return reinterpret_cast<const String8*>(this)->operator<(o);
    }

    CXB_MAYBE_INLINE bool operator>(const String8& o) const {
        return o < *this;
    }

    CXB_INLINE bool contains(String8 needle) const {
        return reinterpret_cast<const String8*>(this)->contains(needle);
    }
    CXB_INLINE bool contains_chars(String8 chars) const {
        return reinterpret_cast<const String8*>(this)->contains_chars(chars);
    }
    CXB_INLINE size_t find(String8 needle) const {
        return reinterpret_cast<const String8*>(this)->find(needle);
    }
    CXB_INLINE String8 trim(String8 chars, bool leading = true, bool trailing = true) const {
        return reinterpret_cast<const String8*>(this)->trim(chars, leading, trailing);
    }

    // ** SECTION: allocator-related methods - delegate to UString
    CXB_MAYBE_INLINE MString8& copy_(Allocator* to_allocator = &heap_alloc) {
        MString8 temp = *this;
        *this = move(this->copy(to_allocator));
        temp.destroy();
        return *this;
    }

    CXB_MAYBE_INLINE MString8 copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        ASSERT(to_allocator != nullptr);

        MString8 result{
            .data = nullptr, .len = len, .not_null_term = not_null_term, .capacity = 0, .allocator = to_allocator};
        if(len > 0) {
            result.reserve(len + not_null_term);
            memcpy(result.data, data, len);
            if(!not_null_term) {
                result.data[len] = '\0';
            }
        }
        return result;
    }

    CXB_MAYBE_INLINE const char* c_str_maybe_copy(Allocator* copy_alloc_if_not) {
        if(not_null_term) {
            ensure_not_null_terminated(copy_alloc_if_not);
        }
        return data;
    }

    CXB_MAYBE_INLINE void destroy() {
        if(data && allocator) {
            allocator->free(data, capacity);
            data = nullptr;
            capacity = 0;
        }
    }

    void reserve(size_t cap) {
        ASSERT(allocator != nullptr);

        size_t old_count = capacity;
        size_t new_count = cap < CXB_STR_MIN_CAP ? CXB_STR_MIN_CAP : cap;
        if(new_count > old_count) {
            data = allocator->realloc(data, old_count, false, new_count);
            capacity = new_count;
        }
    }

    void resize(size_t new_len, char fill_char = '\0') {
        ASSERT(UNLIKELY(allocator != nullptr));

        size_t reserve_size = new_len + !not_null_term;
        if(capacity < reserve_size) {
            reserve(reserve_size);
        }

        size_t old_len = len;
        if(fill_char != '\0' && new_len > old_len) {
            memset(data + old_len, fill_char, new_len - old_len);
        }
        if(!not_null_term) {
            data[new_len] = '\0';
        }
        len = new_len;
    }

    CXB_MAYBE_INLINE void push_back(char ch) {
        if(n_bytes() + 1 >= capacity) {
            reserve(CXB_STR_GROW_FN(capacity));
        }
        data[len] = ch;
        len += 1;
        not_null_term = !(!not_null_term || ch == '\0');
        if(!not_null_term) {
            data[len] = '\0';
        }
    }

    CXB_MAYBE_INLINE char& push() {
        push_back('\0');
        return data[len - 1];
    }

    CXB_MAYBE_INLINE char pop_back() {
        ASSERT(len > 0);
        char ret = data[len - 1];
        len--;
        data[len] = '\0';
        not_null_term = false;
        return ret;
    }

    void extend(String8 other) {
        if(other.len == 0) return;
        reserve(len + other.len);
        memcpy(data + len, other.data, other.len);
        len += other.len;
        if(!not_null_term) {
            data[len] = '\0';
        }
    }

    CXB_MAYBE_INLINE void operator+=(String8 other) {
        this->extend(other);
    }

    CXB_MAYBE_INLINE void extend(const char* str, size_t n = SIZE_MAX) {
        if(!str) {
            return;
        }
        size_t n_len = n == SIZE_MAX ? strlen(str) : n;
        this->extend(String8{.data = const_cast<char*>(str), .len = n_len, .not_null_term = false});
    }

    CXB_MAYBE_INLINE void ensure_not_null_terminated(Allocator* copy_alloc_if_not = nullptr) {
        if(!not_null_term) return;

        ASSERT(allocator != nullptr || copy_alloc_if_not != nullptr);
        if(allocator == nullptr) {
            *this = move(this->copy(copy_alloc_if_not));
        } else {
            this->push_back('\0');
        }
    }
};

/* SECTION: C++-only API */
template <typename T>
struct Atomic {
    static_assert(std::is_integral_v<T> || std::is_pointer_v<T>,
                  "Atomic wrapper only supports integral and pointer types");

    _Atomic(T) value;

    constexpr Atomic(T desired = T{}) noexcept : value(desired) {}

    constexpr Atomic(_Atomic(T) desired) noexcept : value(desired) {}

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;
    Atomic(Atomic&&) = delete;
    Atomic& operator=(Atomic&&) = delete;

    CXB_INLINE void store(T desired, memory_order order = memory_order_seq_cst) noexcept {
        atomic_store_explicit(&value, desired, order);
    }

    CXB_INLINE T load(memory_order order = memory_order_seq_cst) const noexcept {
        return atomic_load_explicit(&value, order);
    }

    CXB_INLINE T exchange(T desired, memory_order order = memory_order_seq_cst) noexcept {
        return atomic_exchange_explicit(&value, desired, order);
    }

    CXB_INLINE bool compare_exchange_weak(T& expected,
                                          T desired,
                                          memory_order success = memory_order_seq_cst,
                                          memory_order failure = memory_order_seq_cst) noexcept {
        return atomic_compare_exchange_weak_explicit(&value, &expected, desired, success, failure);
    }

    CXB_INLINE bool compare_exchange_strong(T& expected,
                                            T desired,
                                            memory_order success = memory_order_seq_cst,
                                            memory_order failure = memory_order_seq_cst) noexcept {
        return atomic_compare_exchange_strong_explicit(&value, &expected, desired, success, failure);
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> fetch_add(
        T arg, memory_order order = memory_order_seq_cst) noexcept {
        return atomic_fetch_add_explicit(&value, arg, order);
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> fetch_sub(
        T arg, memory_order order = memory_order_seq_cst) noexcept {
        return atomic_fetch_sub_explicit(&value, arg, order);
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> fetch_and(
        T arg, memory_order order = memory_order_seq_cst) noexcept {
        return atomic_fetch_and_explicit(&value, arg, order);
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> fetch_or(
        T arg, memory_order order = memory_order_seq_cst) noexcept {
        return atomic_fetch_or_explicit(&value, arg, order);
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> fetch_xor(
        T arg, memory_order order = memory_order_seq_cst) noexcept {
        return atomic_fetch_xor_explicit(&value, arg, order);
    }

    CXB_INLINE operator T() const noexcept {
        return load();
    }

    CXB_INLINE T operator=(T desired) noexcept {
        store(desired);
        return desired;
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> operator++() noexcept {
        return fetch_add(1) + 1;
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> operator++(int) noexcept {
        return fetch_add(1);
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> operator--() noexcept {
        return fetch_sub(1) - 1;
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> operator--(int) noexcept {
        return fetch_sub(1);
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> operator+=(T arg) noexcept {
        return fetch_add(arg) + arg;
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> operator-=(T arg) noexcept {
        return fetch_sub(arg) - arg;
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> operator&=(T arg) noexcept {
        return fetch_and(arg) & arg;
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> operator|=(T arg) noexcept {
        return fetch_or(arg) | arg;
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> operator^=(T arg) noexcept {
        return fetch_xor(arg) ^ arg;
    }

    bool is_lock_free() const noexcept {
        return atomic_is_lock_free(&value);
    }

    static constexpr bool is_always_lock_free = true;
};

struct HeapAllocData {
    Atomic<i64> n_active_bytes;
    Atomic<i64> n_allocated_bytes;
    Atomic<i64> n_freed_bytes;
};
extern HeapAllocData heap_alloc_data;

/* SECTION: containers */
struct AString8 : MString8 {
    AString8(Allocator* allocator = &heap_alloc)
        : MString8{.data = nullptr, .len = 0, .not_null_term = false, .capacity = 0, .allocator = allocator} {}

    AString8(const MString8& m)
        : MString8{.data = m.data,
                   .len = m.len,
                   .not_null_term = m.not_null_term,
                   .capacity = m.capacity,
                   .allocator = m.allocator} {}

    AString8(const char* cstr, size_t n = SIZE_MAX, bool not_null_term = false, Allocator* allocator = &heap_alloc)
        : MString8{.data = nullptr,
                   .len = n == SIZE_MAX ? strlen(cstr) : n,
                   .not_null_term = not_null_term,
                   .capacity = 0,
                   .allocator = allocator} {
        if(this->allocator == nullptr) {
            data = const_cast<char*>(cstr);
        } else {
            reserve(len + 1);
            if(len > 0) {
                memcpy(data, cstr, len);
            }
            data[len] = '\0';
        }
    }

    AString8(AString8&& o)
        : MString8{.data = nullptr, .len = 0, .not_null_term = false, .capacity = 0, .allocator = o.allocator} {
        data = o.data;
        metadata = o.metadata;
        capacity = o.capacity;
        o.allocator = nullptr;
    }

    CXB_INLINE AString8& operator=(AString8&& o) {
        allocator = o.allocator;
        o.allocator = nullptr;
        data = o.data;
        metadata = o.metadata;
        capacity = o.capacity;
        return *this;
    }

    AString8(const AString8& o) = delete;
    AString8& operator=(const AString8& o) = delete;

    CXB_MAYBE_INLINE ~AString8() {
        destroy();
    }

    CXB_MAYBE_INLINE AString8& copy_(Allocator* to_allocator = &heap_alloc) {
        *this = move(this->copy(to_allocator));
        return *this;
    }

    CXB_MAYBE_INLINE AString8 copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        ASSERT(to_allocator != nullptr);

        AString8 result{data, len, not_null_term, to_allocator};
        return result;
    }

    CXB_MAYBE_INLINE MString8 release() {
        MString8 result{
            .data = data, .len = len, .not_null_term = not_null_term, .capacity = capacity, .allocator = allocator};
        this->allocator = nullptr;
        return result;
    }
};

template <typename T>
struct MArray {
    T* data;
    size_t len;
    size_t capacity;
    Allocator* allocator;

    MArray(Allocator* allocator = &heap_alloc) : data{nullptr}, len{0}, capacity{0}, allocator{allocator} {}
    MArray(std::initializer_list<T> xs, Allocator* allocator = &heap_alloc)
        : data{nullptr}, len{0}, capacity{0}, allocator{allocator} {
        extend(Array<T>{(T*) (xs.begin()), xs.size()});
    }
    MArray(T* data, size_t len, Allocator* allocator = &heap_alloc)
        : data{data}, len{len}, capacity{0}, allocator{allocator} {}
    MArray(T* data, size_t len, size_t capacity, Allocator* allocator)
        : data{data}, len{len}, capacity{capacity}, allocator{allocator} {}
    MArray(const MArray<T>& o) = default;
    MArray<T>& operator=(const MArray<T>& o) = default;

    // ** SECTION: slice compatible methods
    CXB_MAYBE_INLINE size_t size() const {
        return len;
    }
    CXB_MAYBE_INLINE bool empty() const {
        return len == 0;
    }
    CXB_MAYBE_INLINE T& operator[](size_t idx) {
        DEBUG_ASSERT(idx < len, "index out of bounds {} >= {}", idx, len);
        return data[idx];
    }
    CXB_MAYBE_INLINE const T& operator[](size_t idx) const {
        DEBUG_ASSERT(idx < len, "index out of bounds {} >= {}", idx, len);
        return data[idx];
    }
    CXB_MAYBE_INLINE T& back() {
        return data[len - 1];
    }
    CXB_MAYBE_INLINE Array<T> slice(i64 i = 0, i64 j = -1) {
        i64 ii = clamp(i < 0 ? (i64) len + i : i, (i64) 0, (i64) len - 1);
        i64 jj = clamp(j < 0 ? (i64) len + j : j, (i64) 0, (i64) len - 1);
        DEBUG_ASSERT(ii <= jj, "i > j: {} ({}) < {} ({})", ii, i, jj, j);

        Array<T> c = *this;
        c.data = c.data + ii;
        c.len = jj - ii + 1;
        return c;
    }

    CXB_MAYBE_INLINE bool operator<(const Array<T>& o) const {
        size_t n = len < o.len ? len : o.len;
        for(size_t i = 0; i < n; ++i) {
            if(o.data[i] < data[i])
                return false;
            else if(data[i] < o.data[i])
                return true;
        }
        return len < o.len;
    }

    CXB_MAYBE_INLINE bool operator==(const Array<T>& o) const {
        if(len != o.len) return false;
        for(size_t i = 0; i < len; ++i) {
            if(!(data[i] == o.data[i])) return false;
        }
        return true;
    }

    CXB_MAYBE_INLINE bool operator!=(const Array<T>& o) const {
        return !(*this == o);
    }

    CXB_MAYBE_INLINE bool operator>(const Array<T>& o) const {
        return o < *this;
    }

    CXB_MAYBE_INLINE operator Array<T>() const {
        return *reinterpret_cast<const Array<T>*>(this);
    }

    // ** SECTION: iterator methods
    CXB_MAYBE_INLINE T* begin() {
        return data;
    }
    CXB_MAYBE_INLINE T* end() {
        return data + len;
    }
    CXB_MAYBE_INLINE const T* begin() const {
        return data;
    }
    CXB_MAYBE_INLINE const T* end() const {
        return data + len;
    }

    // ** SECTION: allocator-related methods
    CXB_MAYBE_INLINE MArray<T>& copy_(Allocator* to_allocator = &heap_alloc) {
        *this = move(this->copy(to_allocator));
        return *this;
    }

    CXB_MAYBE_INLINE MArray<T> copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        ASSERT(to_allocator != nullptr);

        MArray<T> result{nullptr, 0, to_allocator};
        result.reserve(len);
        if(data && len > 0) {
            ::copy(result.data, data, len);
        }
        result.len = len;
        return result;
    }

    CXB_MAYBE_INLINE void destroy() {
        if(data && allocator) {
            ::destroy(data, len);
            allocator->free(data, capacity);
            data = nullptr;
        }
    }

    CXB_MAYBE_INLINE void reserve(size_t cap) {
        ASSERT(allocator != nullptr);

        size_t old_count = capacity;
        size_t new_count = cap < CXB_STR_MIN_CAP ? CXB_STR_MIN_CAP : cap;
        if(new_count > old_count) {
            data = allocator->realloc(data, old_count, false, new_count);
            capacity = new_count;
        }
    }

    CXB_MAYBE_INLINE void resize(size_t new_len) {
        ASSERT(UNLIKELY(allocator != nullptr));

        if(capacity < new_len) {
            reserve(new_len);
        }

        ::construct(data + len, new_len - len);
        len = new_len;
    }

    CXB_MAYBE_INLINE void resize(size_t new_len, T value) {
        ASSERT(UNLIKELY(allocator != nullptr));

        if(capacity < new_len) {
            reserve(new_len);
        }

        ::construct(data + len, new_len - len, value);
        len = new_len;
    }

    CXB_MAYBE_INLINE void push_back(T value) {
        ASSERT(UNLIKELY(allocator != nullptr));
        size_t c = capacity;
        if(len + 1 >= c) {
            c = CXB_SEQ_GROW_FN(c);
            reserve(c);
        }

        data[len++] = move(value);
    }

    CXB_MAYBE_INLINE T& push() {
        reserve(len + 1);
        data[this->len++] = T{};
        return data[this->len - 1];
    }

    CXB_MAYBE_INLINE T pop_back() {
        ASSERT(len > 0);
        T ret = move(data[len - 1]);
        data[len - 1].~T();
        len--;
        return ret;
    }

    CXB_MAYBE_INLINE T& get_or_add_until(size_t idx) {
        if(idx >= len) {
            resize(idx + 1);
        }
        return data[idx];
    }

    template <typename O>
    void release(O& out) {
        out.data = data;
        out.len = len;
        out.capacity = capacity;
        out.allocator = allocator;
        allocator = nullptr;
    }

    void extend(Array<T> other) {
        if(other.len == 0) return;
        reserve(len + other.len);
        ::copy(data + len, other.data, other.len);
        len += other.len;
    }
};

template <typename T, typename O>
static constexpr MArray<T> marray_from_pod(O o, Allocator* allocator) {
    return MArray<T>{o.data, o.len, o.capacity, allocator};
}

template <typename T, typename O>
static constexpr MArray<T> marray_from_pod(O* o, Allocator* allocator) {
    return MArray<T>{o->data, o->len, o->capacity, allocator};
}

template <typename T>
struct AArray : MArray<T> {
    AArray(Allocator* allocator = &heap_alloc) : MArray<T>(allocator) {}
    AArray(T* data, size_t len, Allocator* allocator = &heap_alloc) : MArray<T>(data, len, allocator) {}
    AArray(std::initializer_list<T> xs, Allocator* allocator = &heap_alloc) : MArray<T>(xs, allocator) {}
    AArray(T* data, size_t len, size_t capacity, Allocator* allocator) : MArray<T>(data, len, capacity, allocator) {}
    AArray(const AArray<T>& o) = delete;
    AArray<T>& operator=(const AArray<T>& o) = delete;

    AArray(AArray<T>&& o) : MArray<T>{o.data, o.len, o.capacity, o.allocator} {
        o.allocator = nullptr;
    }
    AArray(MArray<T>&& o) : MArray<T>{o.data, o.len, o.capacity, o.allocator} {
        o.allocator = nullptr;
    }
    AArray<T>& operator=(AArray<T>&& o) {
        this->allocator = o.allocator;
        o.allocator = nullptr;
        this->data = o.data;
        this->len = o.len;
        this->capacity = o.capacity;
        return *this;
    }
    AArray<T>& operator=(MArray<T>&& o) {
        this->allocator = o.allocator;
        o.allocator = nullptr;
        this->data = o.data;
        this->len = o.len;
        this->capacity = o.capacity;
        return *this;
    }

    ~AArray() {
        this->destroy();
    }

    MArray<T> release() {
        MArray<T> self = *this;
        this->allocator = nullptr;
        return self;
    }
};

CXB_C_COMPAT_BEGIN
#define S8_LIT(s) (String8{.data = (char*) &(s)[0], .len = LENGTHOF_LIT(s), .not_null_term = false})
#define S8_DATA(c, l) (String8{.data = (char*) &(c)[0], .len = (l), .not_null_term = false})
#define S8_CSTR(s) (String8{.data = (char*) (s), .len = (size_t) strlen(s), .not_null_term = false})
CXB_C_COMPAT_END

#define S8_STR(s) (String8{.data = (char*) s.c_str(), .len = (size_t) s.size(), .not_null_term = false})

CXB_PURE size_t string8_find(const String8& s, String8 needle) {
    if(needle.len == 0 || needle.len > s.len) {
        return SIZE_MAX;
    }
    for(size_t i = 0; i <= s.len - needle.len; ++i) {
        if(memcmp(s.data + i, needle.data, needle.len) == 0) {
            return i;
        }
    }
    return SIZE_MAX;
}

CXB_PURE bool string8_contains(const String8& s, String8 needle) {
    return string8_find(s, needle) != SIZE_MAX;
}

CXB_PURE bool string8_starts_with(const String8& s, String8 prefix) {
    if(prefix.len > s.len) return false;
    if(prefix.len == 0) return true;
    return memcmp(s.data, prefix.data, prefix.len) == 0;
}

CXB_PURE bool string8_ends_with(const String8& s, String8 suffix) {
    if(suffix.len > s.len) return false;
    if(suffix.len == 0) return true;
    return memcmp(s.data + (s.len - suffix.len), suffix.data, suffix.len) == 0;
}

CXB_PURE static bool _string8_contains_char(String8 chars, char c) {
    for(size_t i = 0; i < chars.len; ++i) {
        if(chars.data[i] == c) {
            return true;
        }
    }
    return false;
}

CXB_PURE bool string8_contains_chars(const String8& s, String8 chars) {
    for(size_t i = 0; i < s.len; ++i) {
        if(_string8_contains_char(chars, s.data[i])) {
            return true;
        }
    }
    return false;
}

CXB_PURE String8 string8_trim(const String8& s, String8 chars, bool leading, bool trailing) {
    size_t start = 0;
    size_t end = s.len;
    if(leading) {
        while(start < end && _string8_contains_char(chars, s.data[start])) {
            start++;
        }
    }
    if(trailing) {
        while(end > start && _string8_contains_char(chars, s.data[end - 1])) {
            end--;
        }
    }
    return s.slice((i64) start, start < end ? (i64) end - 1 : (i64) start - 1);
}

CXB_PURE String8 string8_trim_all(const String8& s, String8 chars, bool leading, bool trailing) {
    if(chars.len == 0) return s;

    size_t start = 0;
    size_t end = s.len;
    if(leading) {
        while(end - start >= chars.len && memcmp(s.data + start, chars.data, chars.len) == 0) {
            start += chars.len;
        }
    }
    if(trailing) {
        while(end - start >= chars.len && memcmp(s.data + (end - chars.len), chars.data, chars.len) == 0) {
            end -= chars.len;
        }
    }
    return s.slice((i64) start, start < end ? (i64) end - 1 : (i64) start - 1);
}

#define MSTRING_NT(a) (MString8{.data = nullptr, .len = 0, .not_null_term = false, .capacity = 0, .allocator = (a)})

#ifdef CXB_IMPL
#include "cxb.cpp"
#endif

// *SECTION*: formatting library
template <typename T, typename... Args>
void _format_impl(Arena* a, String8& dst, const char* fmt, const T& first, const Args&... rest);

template <typename... Args>
String8 format(Arena* a, const char* fmt, const Args&... args) {
    String8 dst = arena_push_string8(a);
    _format_impl(a, dst, fmt, args...);
    return dst;
}

template <typename... Args>
void print(FILE* f, Arena* a, const char* fmt, const Args&... args) {
    String8 str = format(a, fmt, args...);
    if(str.data) {
        fwrite(&str[0], sizeof(char), str.len, f);
    }
}

template <typename... Args>
void write(FILE* f, const char* fmt, const Args&... args) {
    ArenaTmp a = begin_scratch();
    String8 str = format(a.arena, fmt, args...);
    if(str.data) {
        fwrite(&str[0], sizeof(char), str.len, f);
    }
    end_scratch(a);
}

template <typename... Args>
CXB_INLINE void print(const char* fmt, const Args&... args) {
    print(stdout, fmt, args...);
}

template <typename... Args>
CXB_INLINE void println(const char* fmt, const Args&... args) {
    ArenaTmp a = begin_scratch();
    String8 str = format(a.arena, fmt, args...);
    print("{}\n", str);
    end_scratch(a);
}

template <typename... Args>
CXB_INLINE void writeln(FILE* f, const char* fmt, const Args&... args) {
    ArenaTmp a = begin_scratch();
    String8 str = format(a.arena, fmt, args...);
    write(f, "{}\n", str);
    end_scratch(a);
}

/*
to format your own type T, provide an overloaded version of:

void format_value(Arena* a, String8& dst, String8 args, T x);
*/
void format_value(Arena* a, String8& dst, String8 args, const char* s);
void format_value(Arena* a, String8& dst, String8 args, String8 s);

static constexpr char BASE_16_CHARS[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
template <typename T>
void format_value(Arena* a, String8& dst, String8 args, T* s) {
    (void) args;
    constexpr i64 N = (sizeof(u64) * 8);

    u64 v = reinterpret_cast<u64>(s);

    char buf[N] = {};
    int i = 0;
    while(v > 0) {
        buf[i++] = BASE_16_CHARS[(v % 16)];
        v /= 16;
    }

    string8_push_back(dst, a, '0');
    string8_push_back(dst, a, 'x');
    if(i == 0) {
        string8_push_back(dst, a, '0');
    }
    for(i64 j = i - 1; j >= 0; --j) {
        string8_push_back(dst, a, buf[j]);
    }
}

template <typename T>
std::enable_if_t<std::is_integral_v<T>, void> format_value(Arena* a, String8& dst, String8 args, T value) {
    (void) args;
    constexpr i64 N = (sizeof(u64) * 8);

    char buf[N] = {};
    bool neg = value < 0;
    u64 v = neg ? static_cast<u64>(-value) : static_cast<u64>(value);
    int i = 0;
    do {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    } while(v > 0);

    if(neg) {
        string8_push_back(dst, a, '-');
    }
    for(i64 j = i - 1; j >= 0; --j) {
        string8_push_back(dst, a, buf[j]);
    }
}

void format_value(Arena* a, String8& dst, String8 args, bool value);
void format_value(Arena* a, String8& dst, String8 args, f32 value);
void format_value(Arena* a, String8& dst, String8 args, f64 value);

template <typename T, typename... Args>
void _format_impl(Arena* a, String8& dst, const char* fmt, const T& first, const Args&... rest) {
    String8 s = {};
    s.data = (char*) fmt;

    i64 args_i = -1;
    u64 i = 0;
    while(*fmt) {
        s.len += 1;

        char curr = s[i];
        if(curr == '{') {
            args_i = i;
        } else if(curr == '}') {
            String8 args = s.slice(args_i + 1, (i64) i - 1);
            format_value(a, dst, args, first);
            _format_impl(a, dst, s.data + i + 1, rest...);
            break;
        } else if(args_i < 0) {
            string8_push_back(dst, a, curr);
        }
        ++i;
        ++fmt;
    }
}

CXB_MAYBE_INLINE void _format_impl(Arena* a, String8& dst, const char* fmt) {
    while(*fmt) {
        if(*fmt == '{' && *(fmt + 1) != '{') {
            DEBUG_ASSERT(false, "not enough parameters given to format string");
            break;
        }
        string8_push_back(dst, a, *fmt++);
    }
}

struct Utf8Iter;
struct Utf8IterBatch;
CXB_C_EXPORT bool utf8_iter_next(Utf8Iter* iter, Utf8IterBatch* batch);

CXB_C_TYPE struct Utf8Iter {
    CXB_C_COMPAT_BEGIN
    String8 s;
    u64 pos;
    CXB_C_COMPAT_END

    CXB_INLINE bool next(Utf8IterBatch& batch) {
        return utf8_iter_next(this, &batch);
    };
};
CXB_INLINE Utf8Iter make_utf8_iter(String8 s) {
    return Utf8Iter{s, 0};
}

CXB_C_TYPE struct Utf8IterBatch {
    CXB_C_COMPAT_BEGIN
    u32 data[512];
    u64 len;
    CXB_C_COMPAT_END

    Array<u32> as_array() {
        return Array<u32>{&data[0], len};
    }
};

// TODO: with indices?
CXB_MAYBE_INLINE Array<u32> decode_string8(Arena* a, String8 s) {
    Utf8Iter iter = make_utf8_iter(s);
    Utf8IterBatch batch = {};

    Array<u32> codepoints;
    while(iter.next(batch)) {
        codepoints.extend(a, batch.as_array());
    }
    return codepoints;
}

enum HashMapState {
    HM_STATE_EMPTY = 0,
    HM_STATE_OCCUPIED,
    HM_STATE_TOMBSTONE,
};

struct DefaultHasher {
    template <class T>
    size_t operator()(const T& x) const {
        return hash(x);
    }
};

template <typename K, typename V>
struct KvPair {
    K key;
    V value;
};

template <typename K, typename V, typename Hasher = DefaultHasher>
struct MHashMap {
    /* NOTE: key and value are not default constructed, due allocation function in Allocator* */
    struct Entry {
        Kv kv;
        HashMapState state /* NOTE: ZII = HM_STATE_EMPTY */;
    };

    using Kv = KvPair<K, V>;
    using Table = Array<Entry>;

    struct Iterator {
        MHashMap& hm;
        size_t idx;

        Entry& operator*() {
            return hm.table[idx];
        }
        Iterator& ensure_occupied() {
            if(UNLIKELY(hm.table.len == 0)) return *this;
            while(LIKELY(hm.table.len != idx) && hm.table[idx].state != HM_STATE_OCCUPIED) {
                idx = pow2mod(idx + 1, hm.table.len);
            }
            return *this;
        }
        Iterator& operator++() {
            if(UNLIKELY(hm.table.len == 0)) return *this;
            idx = pow2mod(idx + 1, hm.table.len);
            return ensure_occupied();
        }
        bool operator==(const Iterator& it) const {
            return idx == it.idx && LIKELY(&hm == &it.hm);
        }
        bool operator!=(const Iterator& it) const {
            return !(*this == it);
        }
    };

    Table table;
    size_t len;
    Allocator* allocator;
    Hasher hasher;

    MHashMap(Allocator* allocator = &heap_alloc) : table{}, len{0}, allocator{allocator}, hasher{} {}
    explicit MHashMap(size_t bucket_size, Allocator* allocator = &heap_alloc) : MHashMap(allocator) {
        reserve(bucket_size);
    }
    MHashMap(Arena* arena, std::initializer_list<Kv> xs) : MHashMap(push_arena_alloc(arena)) {
        extend(xs);
    }
    MHashMap(std::initializer_list<Kv> xs, Allocator* allocator = &heap_alloc) : MHashMap(allocator) {
        extend(xs);
    }
    MHashMap(const MHashMap&) = delete;
    MHashMap& operator=(const MHashMap&) = delete;

    MHashMap(MHashMap&& o)
        : table{o.table}, len{o.len}, allocator{o.allocator ? o.allocator : &heap_alloc}, hasher{o.hasher} {
        o.table.data = nullptr;
        o.table.len = 0;
        o.len = 0;
        o.allocator = nullptr;
    }
    MHashMap& operator=(MHashMap&& o) {
        if(this != &o) {
            destroy();
            table = o.table;
            len = o.len;
            hasher = o.hasher;
            allocator = o.allocator;
            o.table.data = nullptr;
            o.table.len = 0;
            o.len = 0;
            o.allocator = nullptr;
        }
        return *this;
    }
    ~MHashMap() = default;

    f64 load_factor() const {
        if(table.len == 0) return 0.0;
        return (f64) (len) / (f64) (table.len);
    }
    bool needs_rehash() const {
        return UNLIKELY(!table.data) || load_factor() >= CXB_HM_LOAD_CAP_THRESHOLD;
    }

    template <class T>
    CXB_MAYBE_INLINE size_t _key_hash_index(const T& x) const {
        DEBUG_ASSERT(table.len > 0, "hash map table not initialized");
        size_t h = hasher(x);
        return pow2mod(h, table.len);
    }

    Iterator begin() {
        if(len == 0 || table.len == 0) {
            return Iterator{*this, table.len};
        }
        return Iterator{*this, 0}.ensure_occupied();
    }
    Iterator end() {
        return Iterator{*this, table.len};
    }

    CXB_MAYBE_INLINE bool extend(Array<Kv> xs) {
        for(auto& x : xs) {
            if(!put(x)) return false;
        }
        return true;
    }
    CXB_MAYBE_INLINE bool extend(std::initializer_list<Kv> xs) {
        for(const auto& x : xs) {
            if(!put(x)) return false;
        }
        return true;
    }

    CXB_MAYBE_INLINE void maybe_rehash() {
        if(needs_rehash()) {
            size_t capacity = table.len == 0 ? CXB_HM_MIN_CAP : table.len * 2;
            if(capacity < CXB_HM_MIN_CAP) capacity = CXB_HM_MIN_CAP;
            _reserve(capacity);
        }
    }

    CXB_MAYBE_INLINE void reserve(size_t bucket_size) {
        size_t cap = (size_t) round_up_pow2(bucket_size);
        if(cap < CXB_HM_MIN_CAP) cap = CXB_HM_MIN_CAP;
        if(!table.data || cap > table.len) {
            _reserve(cap);
        }
    }

    CXB_MAYBE_INLINE bool put(Kv kv) {
        maybe_rehash();
        return _insert(::move(kv), true);
    }

    CXB_MAYBE_INLINE bool erase(const K& key) {
        if(!table.data || table.len == 0) return false;
        size_t ii = _key_hash_index(key);
        size_t i = ii;
        do {
            Entry& entry = table[i];
            if(entry.state == HM_STATE_OCCUPIED && entry.kv.key == key) {
                entry.state = HM_STATE_TOMBSTONE;
                ::destroy(&entry.kv.key, 1);
                ::destroy(&entry.kv.value, 1);
                len -= 1;
                return true;
            }
            if(entry.state == HM_STATE_EMPTY) {
                return false;
            }
            i = pow2mod(i + 1, table.len);
        } while(i != ii);
        return false;
    }

    CXB_MAYBE_INLINE const Entry* occupied_entry_for(const K& key) const {
        if(!table.data || table.len == 0) return nullptr;
        size_t ii = _key_hash_index(key);
        size_t i = ii;
        do {
            const Entry& entry = table[i];
            if(entry.state == HM_STATE_OCCUPIED && entry.kv.key == key) {
                return &entry;
            }
            if(entry.state == HM_STATE_EMPTY) {
                return nullptr;
            }
            i = pow2mod(i + 1, table.len);
        } while(i != ii);
        return nullptr;
    }

    CXB_MAYBE_INLINE Entry* occupied_entry_for(const K& key) {
        if(!table.data || table.len == 0) return nullptr;
        size_t ii = _key_hash_index(key);
        size_t i = ii;
        do {
            Entry& entry = table[i];
            if(entry.state == HM_STATE_OCCUPIED && entry.kv.key == key) {
                return &entry;
            }
            if(entry.state == HM_STATE_EMPTY) {
                return nullptr;
            }
            i = pow2mod(i + 1, table.len);
        } while(i != ii);
        return nullptr;
    }

    CXB_MAYBE_INLINE bool contains(const K& key) const {
        return occupied_entry_for(key) != nullptr;
    }

    CXB_MAYBE_INLINE V& operator[](const K& key) {
        Entry* entry = occupied_entry_for(key);
        DEBUG_ASSERT(entry != nullptr, "entry not present");
        return entry->kv.value;
    }

    CXB_MAYBE_INLINE const V& operator[](const K& key) const {
        const Entry* entry = occupied_entry_for(key);
        DEBUG_ASSERT(entry != nullptr, "entry not present");
        return entry->kv.value;
    }

    CXB_MAYBE_INLINE void destroy() {
        if(!table.data || !allocator) return;
        for(size_t i = 0; i < table.len; ++i) {
            if(table[i].state == HM_STATE_OCCUPIED) {
                ::destroy(&table[i].kv.key, 1);
                ::destroy(&table[i].kv.value, 1);
            }
        }
        allocator->free(table.data, table.len);
        table.data = nullptr;
        table.len = 0;
        len = 0;
    }

    CXB_MAYBE_INLINE void _reserve(size_t cap) {
        size_t capacity = cap < CXB_HM_MIN_CAP ? CXB_HM_MIN_CAP : cap;
        DEBUG_ASSERT(round_up_pow2(capacity) == capacity, "{} is not a power of 2", capacity);
        ASSERT(allocator != nullptr);

        Entry* old_data = table.data;
        size_t old_cap = table.len;

        table.data = allocator->calloc<Entry>(0, capacity);
        table.len = capacity;
        size_t old_len = len;
        len = 0;

        if(old_data) {
            for(size_t i = 0; i < old_cap; ++i) {
                if(old_data[i].state == HM_STATE_OCCUPIED) {
                    Kv kv{::move(old_data[i].kv.key), ::move(old_data[i].kv.value)};
                    bool inserted = _insert_into_table(::move(kv), table.data, table.len, false);
                    ASSERT(inserted);
                    ::destroy(&old_data[i].kv.key, 1);
                    ::destroy(&old_data[i].kv.value, 1);
                }
            }
            allocator->free(old_data, old_cap);
            DEBUG_ASSERT(len == old_len, "rehash lost entries");
        }
    }

    CXB_MAYBE_INLINE bool _insert(Kv&& kv, bool check_duplicates) {
        if(!table.data || table.len == 0) {
            _reserve(CXB_HM_MIN_CAP);
        }
        return _insert_into_table(::move(kv), table.data, table.len, check_duplicates);
    }

    CXB_MAYBE_INLINE bool _insert_into_table(Kv&& kv, Entry* data, size_t capacity, bool check_duplicates) {
        DEBUG_ASSERT(data != nullptr && capacity > 0, "hash map table not initialized");
        size_t ii = pow2mod(hasher(kv.key), capacity);
        size_t i = ii;
        size_t tombstone = capacity;

        while(data[i].state != HM_STATE_EMPTY) {
            if(data[i].state == HM_STATE_OCCUPIED) {
                if(check_duplicates && data[i].kv.key == kv.key) {
                    return false;
                }
            } else if(tombstone == capacity) {
                tombstone = i;
            }
            i = pow2mod(i + 1, capacity);
            if(i == ii) {
                return false;
            }
        }

        size_t target = tombstone != capacity ? tombstone : i;
        ::construct(&data[target].kv.key, 1, ::move(kv.key));
        ::construct(&data[target].kv.value, 1, ::move(kv.value));
        data[target].state = HM_STATE_OCCUPIED;
        len += 1;
        return true;
    }
};

template <typename K, typename V, typename Hasher = DefaultHasher>
struct AHashMap : MHashMap<K, V, Hasher> {
    using Base = MHashMap<K, V, Hasher>;
    using Kv = KvPair<K, V>;

    AHashMap(Allocator* allocator = &heap_alloc) : Base{allocator} {}
    explicit AHashMap(size_t bucket_size, Allocator* allocator = &heap_alloc) : Base{allocator} {
        this->reserve(bucket_size);
    }
    AHashMap(std::initializer_list<Kv> xs, Allocator* allocator = &heap_alloc) : Base{allocator} {
        this->extend(xs);
    }
    AHashMap(const AHashMap&) = delete;
    AHashMap& operator=(const AHashMap&) = delete;

    AHashMap(AHashMap&& o) : Base{o.allocator ? o.allocator : &heap_alloc} {
        this->table = o.table;
        this->len = o.len;
        this->hasher = o.hasher;
        this->allocator = o.allocator;
        o.table.data = nullptr;
        o.table.len = 0;
        o.len = 0;
        o.allocator = nullptr;
    }
    AHashMap(Base&& o) : Base{o.allocator ? o.allocator : &heap_alloc} {
        this->table = o.table;
        this->len = o.len;
        this->hasher = o.hasher;
        this->allocator = o.allocator;
        o.table.data = nullptr;
        o.table.len = 0;
        o.len = 0;
        o.allocator = nullptr;
    }

    AHashMap& operator=(AHashMap&& o) {
        if(this != &o) {
            this->destroy();
            this->table = o.table;
            this->len = o.len;
            this->hasher = o.hasher;
            this->allocator = o.allocator;
            o.table.data = nullptr;
            o.table.len = 0;
            o.len = 0;
            o.allocator = nullptr;
        }
        return *this;
    }
    AHashMap& operator=(Base&& o) {
        this->destroy();
        this->table = o.table;
        this->len = o.len;
        this->hasher = o.hasher;
        this->allocator = o.allocator;
        o.table.data = nullptr;
        o.table.len = 0;
        o.len = 0;
        o.allocator = nullptr;
        return *this;
    }

    ~AHashMap() {
        this->destroy();
    }

    Base release() {
        Base out{this->allocator ? this->allocator : &heap_alloc};
        out.table = this->table;
        out.len = this->len;
        out.hasher = this->hasher;
        out.allocator = this->allocator;

        this->allocator = nullptr;
        this->table.data = nullptr;
        this->table.len = 0;
        this->len = 0;
        return out;
    }
};

inline String8 operator""_s8(const char* s, size_t len) {
    return String8{.data = (char*) s, .len = len, .not_null_term = false};
}

#endif
