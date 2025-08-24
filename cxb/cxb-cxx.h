/*
# cxb: Base library for CX (Orthodox-C++)

## Inspiration
* Odin
* Zig
* Python
* Nim

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

## Containers
* C & C++ compatible: use these types when defining a C API
    * `String8`: a pointer to char* (`data`), a length (`len`), and a flag (`not_null_term`) to indicate if the string
is null-terminated
        - `String8` is a POD type, it does not own the memory it points to, it is only a view into a contiguous
block of memory
        - In C-land: free functions are provided, such as TODO
        - In C++-land: there are methods the same operations the C free functions
          support plus operator overloads for convenience, e.g. `operator[]`,
          `slice`, `c_str`, `empty`, `size`, `n_bytes`, etc.
    * `MString`: a manually memory managed string, "M" stands for "manual"
        - This type is a `std::string` alternative, but requires manual memory management
        - Think of this type as a `String8` that is optionally attached to an allocator
        - Compatible with `String8`
        - In C-land: free functions are provided, such as TODO
        - In C++-land: there are methods the same operations the C free functions
          support plus operator overloads for convenience, e.g. `operator[]`, `c_str`, `empty`, `size`, `n_bytes`, etc.
        - Call `.destroy()` (in C++) or `cxb_mstring8_destroy` (in C) to free the memory
        - Destructor does not call `.destroy()`, see `String` for this functionality
* C++ only types: use these types when when defining C++ APIs or in implementation files
    * `AString: an automatically managed string using RAII, compatible with MString
        - This type is a `std::string` alternative, with RAII semantics but requires manual copies
        - This type is an extension of `MString`, which automatically calls `destroy()` on destruction
        - This type is compatible with `String8`
        - Copies must be done manually via `copy()`, i.e. the copy contractor and assignment operator are deleted
        - Moves are supported
        - Call `.release()` to remove ownership of the memory, i.e. such that the destructor does not call `destroy()`
    * `Array<T>`: similar to String8 but for any type
    * `MArray<T>`: a manually memory managed expandable sequence of elements where elements are stored contiguously in
memory, "M" stands for "manual"
        - Provides an interface similar to `MString`, but for any type; "null terminated" is not supported
        - This type is a `std::vector<T>` alternative, but requires manual memory management
        - This type is compatible with `Array<T>`
        - Call `.destroy()` to free memory
        - Destructor does not call `.destroy()`, see `AArray<T>` for this functionality
    * `AArray<T>`: an automatically managed sequence container using RAII, compatible with `MArray<T>`
        - Provides an interface similar to `String`, but for any type; "null terminated" is not supported
        - This type is a `std::vector<T>` alternative, with RAII semantics but requires manual copies
        - This type is an extension of `MArray<T>`, which automatically calls `destroy()` on destruction
        - This type is compatible with `Array<T>`
        - Copies must be done manually via `copy()`, i.e. the copy contractor and assignment operator are deleted
        - Moves are supported
        - Call `.release()` to remove ownership of the memory, i.e. such that the destructor does not call `destroy()`
*/

#ifndef CXB_H
#define CXB_H

#ifndef __cplusplus
#error "Include <cxb/cxb-c.h> when compiling C code. <cxb/cxb-cxx.h> is C++-only."
#endif

/* SECTION: configuration */
#if __cpp_concepts
#define CXB_USE_CXX_CONCEPTS
#endif

#define CXB_SEQ_MIN_CAP 32
#define CXB_SEQ_GROW_FN(x) (x) + (x) / 2 /* 3/2, reducing chance to overflow */
#define CXB_STR_MIN_CAP 32
#define CXB_STR_GROW_FN(x) (x) + (x) / 2 /* 3/2, reducing chance to overflow  */

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
#include <string.h>
CXB_C_COMPAT_END

#include <limits>
#include <new>
#include <type_traits> // 27ms

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

#define LIKELY(x) x
#define UNLIKELY(x) x

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

// NOTE: GCC doesn't support _Atomic in C++
#if defined(__STDC_NO_ATOMICS__) || (defined(__GNUC__) && !defined(__clang__))
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
CXB_PURE const T& clamp(const T& x, const T& a, const T& b) {
    return a < b ? max(min(b, x), a) : min(max(a, x), b);
}

template <typename T>
static inline void swap(T& t1, T& t2) noexcept {
    T temp(move(t1));
    t1 = move(t2);
    t2 = move(temp);
}

/* SECTION: arena */
struct Arena;
struct String8;
template <typename T>
struct Array;

#ifdef CXB_USE_CXX_CONCEPTS
template <typename A, typename T>
concept ArrayLike = requires(A x) {
    { x.data } -> std::convertible_to<T*>;
    { x.len } -> std::convertible_to<u64>;
};

template <typename A>
concept ArrayLikeNoT = requires(A x) {
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
};

CXB_C_EXPORT Arena* arena_make(ArenaParams params);
CXB_C_EXPORT Arena* arena_make_nbytes(size_t n_bytes);
CXB_C_EXPORT void arena_destroy(Arena* arena);

CXB_C_EXPORT void* arena_push_bytes(Arena* arena, size_t size, size_t align);
CXB_C_EXPORT void arena_pop_to(Arena* arena, u64 pos);
CXB_C_EXPORT void arena_clear(Arena* arena);

CXB_C_TYPE struct ArenaTemp {
    CXB_C_COMPAT_BEGIN
    Arena* arena;
    u64 pos;
    CXB_C_COMPAT_END
};

Arena* get_perm();
ArenaTemp begin_scratch();
void end_scratch(const ArenaTemp& tmp);

struct AArenaTemp : ArenaTemp {
    AArenaTemp(const ArenaTemp& other) : ArenaTemp{other} {}
    CXB_INLINE ~AArenaTemp() {
        end_scratch(*this);
    }
};

// *SECTION: Arena free functions
template <typename T>
inline T* arena_push(Arena* arena, size_t n = 1) {
    const size_t size = sizeof(T) * n;
    T* data = (T*) arena_push_bytes(arena, size, alignof(T));
    if constexpr(!std::is_trivially_default_constructible_v<T>) {
        for(size_t i = 0; i < n; ++i) {
            new(data + i) T{};
        }
    } else {
        memset(data, 0, size);
    }
    return data;
}

template <typename T>
inline T* arena_push(Arena* arena, T value, size_t n = 1) {
    const size_t size = sizeof(T);
    T* data = (T*) arena_push_bytes(arena, size, alignof(T));
    if constexpr(!std::is_trivially_default_constructible_v<T>) {
        for(size_t i = 0; i < n; ++i) {
            new(data + i) T{value};
        }
    } else {
        for(size_t i = 0; i < n; ++i) {
            memcpy((void*) (data + i), (void*) (&value), sizeof(T));
        }
    }
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
    return Array<T>{.data = data, .len = n};
}

template <typename T>
inline Array<T> arena_push_array(Arena* arena, Array<T> to_copy) {
    T* data = arena_push<T>(arena, to_copy.len);
    if constexpr(std::is_trivially_copyable_v<T>) {
        memcpy(data, to_copy.data, to_copy.len * sizeof(T));
    } else {
        for(size_t i = 0; i < to_copy.len; ++i) {
            data[i] = to_copy.data[i];
        }
    }
    return Array<T>{.data = data, .len = to_copy.len};
}

template <typename A, typename T>
inline void array_push_back(A& xs, Arena* arena, T value)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A, T>
#endif
{
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) ||
                     (void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end,
                 "array not allocated on arena");
    DEBUG_ASSERT(UNLIKELY(xs.data == nullptr) || (void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos),
                 "cannot push unless array is at the end");
    void* data = arena_push_bytes(arena, sizeof(T), alignof(T));
    xs.data = UNLIKELY(xs.data == nullptr) ? (T*) data : xs.data;
    xs.data[xs.len] = value;
    xs.len += 1;
}

#ifdef CXB_USE_CXX_CONCEPTS
template <typename A, typename T, typename... Args>
#else
template <typename A, typename... Args>
#endif
inline void array_emplace_back(A& xs, Arena* arena, Args&&... args)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A, T>
#endif
{
#ifndef CXB_USE_CXX_CONCEPTS
    using T = decltype(xs.data[0]);
#endif

    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot push unless array is at the end");
    arena_push_bytes(arena, sizeof(T), alignof(T));
    xs.data[xs.len] = T{forward<Args>(args)...};
    xs.len += 1;
}

#ifdef CXB_USE_CXX_CONCEPTS
template <typename A, typename T>
#else
template <typename A>
#endif
inline void array_pop_back(A& xs, Arena* arena)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLike<A, T>
#endif
{
#ifndef CXB_USE_CXX_CONCEPTS
    using T = decltype(xs.data[0]);
#endif
    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot pop unless array is at the end");
    arena_pop_to(arena, arena->pos - sizeof(xs.data));

    if constexpr(!std::is_trivially_destructible_v<T>) {
        xs.data[xs.len].~T();
    }
    xs.len -= 1;
}

template <typename A, typename B>
inline void array_insert(A& xs, Arena* arena, const B& to_insert, size_t i)
    requires ArrayLikeNoT<A> && ArrayLikeNoT<B>
{
    using T = decltype(xs.data[0]);
    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot push unless array is at the end");
    ASSERT(i <= xs.len, "insert position out of bounds");

    arena_push_bytes(arena, to_insert.len * sizeof(T), alignof(T));

    size_t old_len = xs.len;
    xs.len += to_insert.len;

    memmove(xs.data + i + to_insert.len, xs.data + i, (old_len - i) * sizeof(T));
    memcpy(xs.data + i, to_insert.data, to_insert.len * sizeof(T));
}

template <typename A, typename B>
inline void array_extend(A& xs, Arena* arena, const B& to_append)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLikeNoT<A> && ArrayLikeNoT<B>
#endif
{
    using T = decltype(xs.data[0]);
    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot push unless array is at the end");

    arena_push_bytes(arena, to_append.len * sizeof(T), alignof(T));

    size_t old_len = xs.len;
    xs.len += to_append.len;

    memcpy(xs.data + old_len, to_append.data, to_append.len * sizeof(T));
}

template <typename A>
inline void array_pop_all(A& xs, Arena* arena)
#ifdef CXB_USE_CXX_CONCEPTS
    requires ArrayLikeNoT<A>
#endif
{
    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot pop unless array is at the end");
    arena_pop_to(arena, arena->pos - (sizeof(xs.data[0]) * xs.len));
    xs.data = nullptr;
    xs.len = 0;
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
        i64 ii = clamp(i < 0 ? (i64) len + i : i, (i64) 0, len == 0 ? 0 : (i64) len - 1);
        i64 jj = clamp(j < 0 ? (i64) len + j : j, (i64) 0, len == 0 ? 0 : (i64) len - 1);

        String8 c = *this;
        c.data = c.data + ii;
        c.len = max(0ll, jj - ii + 1);
        c.not_null_term = ii + c.len == len ? this->not_null_term : true;
        return c;
    }

    CXB_MAYBE_INLINE const char* c_str() const {
        return not_null_term ? nullptr : data;
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
CXB_MAYBE_INLINE std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, ParseResult<T>> string8_parse(
    String8 str, u64 base = 10) {
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
struct Array {
    T* data;
    size_t len;

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
        array_insert(*this, arena, value, i);
    }
    CXB_INLINE void insert(Arena* arena, Array<T> to_insert, size_t i) {
        array_insert(*this, arena, to_insert, i);
    }
    CXB_INLINE void extend(Arena* arena, Array<T> to_append) {
        array_extend(*this, arena, to_append);
    }
};

// *SECTION*: formatting library

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

CXB_C_TYPE struct MString {
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

    // ** SECTION: allocator-related methods - delegate to UString
    CXB_MAYBE_INLINE MString& copy_(Allocator* to_allocator = &heap_alloc) {
        MString temp = *this;
        *this = move(this->copy(to_allocator));
        temp.destroy();
        return *this;
    }

    CXB_MAYBE_INLINE MString copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        ASSERT(to_allocator != nullptr);

        MString result{
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
struct AString : MString {
    AString(Allocator* allocator = &heap_alloc)
        : MString{.data = nullptr, .len = 0, .not_null_term = false, .capacity = 0, .allocator = allocator} {}

    AString(const MString& m)
        : MString{.data = m.data,
                  .len = m.len,
                  .not_null_term = m.not_null_term,
                  .capacity = m.capacity,
                  .allocator = m.allocator} {}

    AString(const char* cstr, size_t n = SIZE_MAX, bool not_null_term = false, Allocator* allocator = &heap_alloc)
        : MString{.data = nullptr,
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

    AString(AString&& o)
        : MString{.data = nullptr, .len = 0, .not_null_term = false, .capacity = 0, .allocator = o.allocator} {
        data = o.data;
        metadata = o.metadata;
        capacity = o.capacity;
        o.allocator = nullptr;
    }

    CXB_INLINE AString& operator=(AString&& o) {
        allocator = o.allocator;
        o.allocator = nullptr;
        data = o.data;
        metadata = o.metadata;
        capacity = o.capacity;
        return *this;
    }

    AString(const AString& o) = delete;
    AString& operator=(const AString& o) = delete;

    CXB_MAYBE_INLINE ~AString() {
        destroy();
    }

    CXB_MAYBE_INLINE AString& copy_(Allocator* to_allocator = &heap_alloc) {
        *this = move(this->copy(to_allocator));
        return *this;
    }

    CXB_MAYBE_INLINE AString copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        ASSERT(to_allocator != nullptr);

        AString result{data, len, not_null_term, to_allocator};
        return result;
    }

    CXB_MAYBE_INLINE MString release() {
        MString result{
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
            if constexpr(std::is_trivially_assignable_v<T, T>) {
                memcpy(result.data, data, len * sizeof(T));
            } else {
                for(size_t i = 0; i < len; ++i) {
                    data[i] = result.data[i];
                }
            }
        }
        result.len = len;
        return result;
    }

    CXB_MAYBE_INLINE void destroy() {
        if(data && allocator) {
            if constexpr(!std::is_trivially_destructible_v<T>) {
                for(size_t i = 0; i < len; ++i) {
                    data[i].~T();
                }
            }
            allocator->free(data, capacity);
            data = nullptr;
        }
    }

    CXB_MAYBE_INLINE void reserve(size_t cap) {
        ASSERT(allocator != nullptr);

        size_t old_count = capacity;
        size_t new_count = cap < CXB_STR_MIN_CAP ? CXB_STR_MIN_CAP : cap;
        if(new_count > old_count) {
            data = allocator->realloc(data, old_count, std::is_trivially_default_constructible_v<T>, new_count);
            capacity = new_count;
        }
    }

    CXB_MAYBE_INLINE void resize(size_t new_len) {
        ASSERT(UNLIKELY(allocator != nullptr));

        if(capacity < new_len) {
            reserve(new_len);
        }

        if constexpr(!std::is_trivially_default_constructible_v<T>) {
            for(size_t i = len; i < new_len; ++i) {
                new(data + i) T{};
            }
        }

        len = new_len;
    }

    CXB_MAYBE_INLINE void resize(size_t new_len, T value) {
        ASSERT(UNLIKELY(allocator != nullptr));

        if(capacity < new_len) {
            reserve(new_len);
        }

        // TODO: optimize
        for(size_t i = len; i < new_len; ++i) {
            new(data + i) T{value};
        }
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
        // TODO std::copy alternative
        if constexpr(std::is_trivially_assignable_v<T, T>) {
            memmove(data + len, other.data, other.len);
            len += other.len;
        } else {
            size_t old_len = len;
            len += other.len;
            for(size_t i = old_len; i < len; ++i) {
                data[i] = other[i];
            }
        }
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

#define MSTRING_NT(a) (MString{.data = nullptr, .len = 0, .not_null_term = false, .capacity = 0, .allocator = (a)})

#ifdef CXB_IMPL
#include "cxb.cpp"
#endif

template <typename T, typename... Args>
void _format_impl(Arena* a, String8& dst, const char* fmt, const T& first, const Args&... rest);

template <typename... Args>
String8 format(Arena* a, const char* fmt, const Args&... args) {
    String8 dst = arena_push_string8(a);
    _format_impl(a, dst, fmt, args...);
    return dst;
}

template <typename... Args>
String8 format(const char* fmt, const Args&... args) {
    ArenaTemp a = begin_scratch();
    String8 result = format(a.arena, fmt, args...);
    end_scratch(a);
    return result;
}

template <typename... Args>
void print(FILE* f, Arena* a, const char* fmt, const Args&... args) {
    String8 str = format(a, fmt, args...);
    if(str.data) {
        fwrite(&str[0], sizeof(char), str.len, f);
    }
}

template <typename... Args>
void print(FILE* f, const char* fmt, const Args&... args) {
    String8 str = format(fmt, args...);
    if(str.data) {
        fwrite(&str[0], sizeof(char), str.len, f);
    }
}

template <typename... Args>
CXB_INLINE void print(const char* fmt, const Args&... args) {
    print(stdout, fmt, args...);
}

template <typename... Args>
CXB_INLINE void println(const char* fmt, const Args&... args) {
    auto str = format(fmt, args...);
    print("{}\n", str);
}

/*
to format your own type T, provide an overloaded version of:

void format_value(Arena* a, String8& dst, String8 args, T x);
*/
void format_value(Arena* a, String8& dst, String8 args, const char* s);
void format_value(Arena* a, String8& dst, String8 args, String8 s);

template <class T>
std::enable_if_t<std::is_integral_v<T>, void> format_value(Arena* a, String8& dst, String8 args, T value) {
    char buf[sizeof(T) * 8] = {};
    bool neg = value < 0;
    u64 v = neg ? static_cast<u64>(-value) : static_cast<u64>(value);
    int i = 0;
    while(v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }

    if(neg) {
        string8_push_back(dst, a, '-');
    }
    for(int j = i - 1; j >= 0; --j) {
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

#endif
