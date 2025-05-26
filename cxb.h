/*
# cxb: Base library for CX (Orthodox-C++)

This library is my own style (Miguel's) of writing C++. This does include RAII
by default, but it can be disabled. Please see below in the "configuration"
section.

Inspiration:
- Nim
- Zig
- Python

# Containers
* Seq<T>

*/

#pragma once

/* SECTION: configuration */
// #define CXB_DISABLE_RAII
// #define CXB_ALLOC_TEMPLATE
// #define CXB_NO_NAMESPACE
#define CXB_MALLOCATOR_MIN_CAP 32
#define CXB_MALLOCATOR_GROW_FN(x) (x) + (x)/2  /* 3/2 without overflow */

#ifdef CXB_ALLOC_TEMPLATE
#error "CXB_ALLOC_TEMPLATE unsupported"
#endif

/* SECTION: includes */
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h> // TODO: removeme
#include <stdio.h>
#include <type_traits> // 27ms
/* NOTE: #include <utility>  // 98ms */


/* SECTION: macros */
// NOTE some macros are copied/modified from Blend2D, see: https://github.com/blend2d/blend2d/blob/bae3b6c600186a69a9f212243ed9700dc93a314a/src/blend2d/api.h#L563
#define CXB_EXPORT
#define CXB_INTERNAL static
#define CXB_FORCE_INLINE __attribute__((always_inline))

#ifdef CXB_NO_NAMESPACE
#define CXB_NS_BEGIN 
#define CXB_NS_END
#else
#define CXB_NS_BEGIN namespace cxb {
#define CXB_NS_END }
#endif

#define COUNTOF_LIT(a)    (size_t)(sizeof(a) / sizeof(*(a)))
#define LENGTHOF_LIT(s)   (countof(s) - 1)
#define ASSERT(x, msg) if(!(x)) __builtin_debugtrap()
#define REQUIRES(x) if(!(x)) __builtin_debugtrap()
#define LIKELY(x) x
#define UNLIKELY(x) x
//#define INFO(msg)
//#define WARN(msg)
//#define FATAL(msg)

#if defined(__GNUC__)
  #define CXB_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
  #define CXB_INLINE __forceinline
#else
  #define CXB_INLINE inline
#endif

#if defined(__clang__)
  #define CXB_INLINE_NODEBUG inline __attribute__((__always_inline__, __nodebug__))
#elif defined(__GNUC__)
  #define CXB_INLINE_NODEBUG inline __attribute__((__always_inline__, __artificial__))
#else
  #define CXB_INLINE_NODEBUG CXB_INLINE
#endif


#define CXB_COMP constexpr CXB_INLINE_NODEBUG
#define CXB_COMP_INLINE constexpr CXB_INLINE

#if defined(__clang_major__) && __clang_major__ >= 6
  #define CXB_PURE CXB_COMP __attribute__((__pure__))
#elif defined(__GNUC__) && __GNUC__ >= 6
  #define CXB_PURE CXB_COMP __attribute__((__pure__))
#else
  #define CXB_PURE CXB_COMP
#endif

/* * SECTION: primitives */
CXB_NS_BEGIN

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

// TODO: ifdef on clang/gcc
typedef __uint128_t u128;
typedef __int128_t i128;

/* SECTION: primitive functions */
template <class T> CXB_PURE T min(const T& a, const T& b) { return a < b ? a : b; }
template <class T> CXB_PURE T max(const T& a, const T& b) { return a > b ? a : b; }
template <class T> CXB_PURE T clamp(const T& x, const T& a, const T& b) { 
    REQUIRES(a < b);
    return max(min(b, x), a);
}


template<typename T>
CXB_PURE typename std::remove_reference<T>::type&& move(T&& v) noexcept { return static_cast<typename std::remove_reference<T>::type&&>(v); }

template<typename T>
CXB_PURE T&& forward(typename std::remove_reference<T>::type& v) noexcept { return static_cast<T&&>(v); }

template<typename T>
CXB_PURE T&& forward(typename std::remove_reference<T>::type&& v) noexcept { return static_cast<T&&>(v); }

template<typename T>
CXB_COMP_INLINE void swap(T& t1, T& t2) noexcept {
  T temp(move(t1));
  t1 = move(t2);
  t2 = move(temp);
}

// TODO: placement new?
// inline void *operator new(size_t, void *p) noexcept { return p; }

struct Allocator {
    size_t (*growth_sug_impl)(const Allocator* a, size_t count);
    void* (*alloc_impl)(Allocator* a, bool fill_zeros, void* head, size_t num_bytes, size_t alignment, size_t old_n_bytes);
    void (*free_impl)(Allocator* a, void* head);

    template <class T, class H>
    struct AllocationWithHeader {
        T* data;
        H* header;
    };

    // suggested growth
    inline size_t growth_sug(size_t count) const { return this->growth_sug_impl(this, count); }
    inline size_t min_count_sug() const { return this->growth_sug_impl(this, 0); }

    template <class T, class ...Args>
    inline T* alloc(size_t count = 1, Args&& ...args) {
        T* result = (T*)this->alloc_impl(this, false, nullptr, sizeof(T) * count, alignof(T), 0);
        for(size_t i = 0; i < count; ++i) new (result + i) T{args...};
        return result;
    }

    template <class T>
    inline T* calloc(size_t old_count, size_t count = 1) {
        T* result = (T*)this->alloc_impl(this, true, nullptr, sizeof(T) * count, alignof(T), sizeof(T) * old_count);
        return result;
    }

    template <class T, class ...Args>
    inline T* realloc(T* head, size_t old_count, size_t count = 1, Args&& ...args) {
        T* result = (T*)this->alloc_impl(this, false, (void*)head, sizeof(T) * count, alignof(T), sizeof(T) * old_count);
        // TODO: std::forward?
        for(size_t i = old_count; i < count; ++i) {
            // new (result+i) T{(Args &&)args...};
            // new (result+i) T{std::forward<Args...>(args...)};
            new (result+i) T{cxb::forward<Args>(args)...};
        }
        return result;
    }

    template <class H, class T, class ...Args>
    inline AllocationWithHeader<T, H> realloc_with_header(H* header, size_t old_count, size_t count, Args&& ...args) {
        char* new_header = (char*)this->alloc_impl(
            this,                              // allocator
            false,                             // fill_zeros
            (void*)header,                     // header
            sizeof(T) * count + sizeof(H),     // num_bytes
            alignof(T) + sizeof(H),            // alignment
            sizeof(T) * old_count + sizeof(H)  // old bytes
        );
        T* data = (T*)(new_header + sizeof(H));

        // TODO: std::forward?
        for(size_t i = old_count; i < count; ++i) {
            // new (data+i) T{(Args &&)args...};
            // new (data+i) T{std::forward<Args...>(args...)};
            new (data+i) T{cxb::forward<Args>(args)...};
        }
        return AllocationWithHeader<T, H>{data, (H*)new_header};
    }

    template <class H, class T>
    inline AllocationWithHeader<T, H> recalloc_with_header(H* header, size_t old_count, size_t count) {
        char* new_header = (char*)this->alloc_impl(
            this,                                               // allocator
            true,                                               // fill_zeros
            (void*)header,                                      // header
            sizeof(T) * count + sizeof(H),                      // num_bytes
            alignof(T) + sizeof(H),                             // alignment
            sizeof(T) * old_count + sizeof(H) * (old_count > 0) // old bytes
        );
        T* data = (T*)(new_header + sizeof(H));
        return AllocationWithHeader<T, H>{data, (H*)new_header};
    }


    template <class T>
    inline void free(T* head) {
        this->free_impl(this, (void*)head);
    }
};


struct Mallocator: Allocator {
    Mallocator();
};

struct Arena: Allocator {
    Arena();
};

extern thread_local Mallocator default_alloc;


/* SECTION: containers */
template <class T>  // NOTE: could use allocator as template param
struct Seq {
    Allocator* allocator;
    T* data;
    size_t len;

    Seq(Allocator* allocator = &default_alloc) : allocator{allocator}, data{nullptr}, len{0} { reserve(0); }
    Seq(T* data, size_t n = 0, Allocator* allocator = &default_alloc) : allocator{allocator}, data{data}, len{n} { reserve(0); }
#ifndef CXB_DISABLE_RAII
    CXB_FORCE_INLINE ~Seq() { destroy(); }
#endif

    // creates a slice
    Seq(const Seq<T>& o) : allocator{nullptr}, data{o.data}, len{o.len} { }

    // ** SECTION: slice compatible methods
    inline size_t size() const { return len; }
    inline bool empty() const { return len == 0; }
    inline T& operator[](size_t idx) { return data[idx]; }
    inline const T& operator[](size_t idx) const { return data[idx]; }
    inline T& back() { return data[len-1]; }
    inline Seq<T> slice(size_t i, size_t j = 0) {
        Seq<T> c = *this;
        c.data = c.data + i;
        c.len = j == 0 ? c.len : j - i + 1;
        return c;
    }


    // ** SECTION: allocating methods
    inline Seq<T> copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        REQUIRES(to_allocator != nullptr);
        Seq<T> result{nullptr, 0, to_allocator};
        // TODO
        return result;
    }

    inline size_t* start_mem() { 
        if(this->data == nullptr) return nullptr;
        return ((size_t*)this->data) - 1;
    }

    inline size_t& _capacity() { REQUIRES(data); return *start_mem(); }

    inline size_t capacity() { 
        if(!data) return 0;
        return *start_mem();
    }

    inline void destroy() {
        if(data && allocator) { 
            allocator->free(start_mem());
            data = nullptr; 
        }
    }

    inline void reserve(size_t cap) { 
        size_t* alloc_mem = start_mem();
        size_t old_count = capacity();
        size_t new_count = max(cap, allocator->min_count_sug());
        auto mem = allocator->recalloc_with_header<size_t, T>(
            alloc_mem,
            old_count,
            new_count
        );
        data = mem.data;
        _capacity() = new_count;
    }

    template <class... Args>
    inline void resize(size_t new_len, Args&&... args) { 
        if(capacity() < new_len) {
            reserve(new_len);
        }
        for(int i = len; i < new_len; ++i) {
            // TODO: std::forward?
            // new (data+i) T{(Args &&)args...};
            // new (data+i) T{std::forward<Args...>(args)...)};
            new (data+i) T{cxb::forward<Args>(args)...};
        }
        len = new_len;
    }

    inline void push_back(T value) { 
        REQUIRES(UNLIKELY(allocator != nullptr));
        size_t c = capacity();
        if(this->len >= c) {
            c = allocator->growth_sug(c);
            reserve(c);
        }

        data[this->len++] = value;
    }

    inline T& push() {
        size_t c = capacity();
        if(this->len >= c) {
            c = allocator->growth_sug(c);
            reserve(c);
        }

        data[this->len++] = T{};
        return data[this->len - 1];
    }

    inline T pop_back() {
        T ret = data[len - 1];
        len--;
        return ret;
    }

    inline T& get_or_add_until(size_t idx) {
        if(idx >= len) {
            resize(idx + 1);
        }
        return data[idx];
    }
};


struct Str8 {
    char* data;
    size_t len;

    inline size_t size() const { return len; }
    inline char& operator[](size_t idx) { return data[idx]; }
    inline const char& operator[](size_t idx) const { return data[idx]; }
};

// * SECTION: math
// TODO
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

static const Mat44f identity4x4 = {
    .arr = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }
};
static const Mat33f identity3x3 = {
    .arr = {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1,
    }
};


// * SECTION: variants
// TODO: eval if wanted
template <class T>
struct Optional {
    // static constexpr const Optional<T> None = Optional<T>{T{}, false};
    T value;
    bool exists;
};

CXB_NS_END

#ifdef CXB_IMPL
#include "cxb.cpp"
#endif

#define s8_lit(s) (Str8{(char *)&s[0], lengthof(s)})
#define s8_str(s) (Str8{(char *)&s[0], (size_t)s.size()})
#define s8_cstr(s) (Str8{(char *)&s[0], (size_t)strlen(s)})
