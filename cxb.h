#pragma once

// * SECTION: includes
#include <cstddef>
#include <stdint.h>
#include <stdlib.h> // TODO: removeme

// * SECTION: constants
// TODO: constexpr?
#define SEQ_MIN_CAP 32
#define SEQ_GROWTH_FACTOR 2

// * SECTION: macros
#define CXB_EXPORT
#define CXB_INTERNAL static
#define CXB_FORCE_INLINE __attribute__((always_inline))

#define COUNTOF_LIT(a)    (size_t)(sizeof(a) / sizeof(*(a)))
#define LENGTHOF_LIT(s)   (countof(s) - 1)
#define ASSERT(x, msg) if(x) __builtin_debugtrap()
#define REQUIRES(x) if(x) __builtin_debugtrap()
//#define INFO(msg)
//#define WARN(msg)
//#define FATAL(msg)

// * SECTION: primitives
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

CXB_EXPORT int foo();

struct Allocator {
    // TODO: error?
    void* (*alloc)(size_t num_bytes, size_t alignment);
    void* (*realloc)(void* head, size_t num_bytes, size_t alignment);
    void (*free)(void* head);

    // TODO: templates
    // template <class T>
    // T* alloc()
};

// * SECTION: containers
template <class T>
struct Seq {
    T* data;
    size_t len;

    Seq() : data{nullptr}, len{0} { reserve(SEQ_MIN_CAP); }
    Seq(size_t n) : data{nullptr}, len{0} { reserve(n); }
    ~Seq() { destroy(); }

    inline size_t* start_mem() { 
        REQUIRE(this->data != nullptr);
        return ((size_t*)this->data) - 1;
    }
    inline size_t reserve_size(size_t capacity) { return capacity * sizeof(T) + sizeof(size_t); }
    inline size_t& _capacity() { return *start_mem(); }
    inline size_t capacity() { return *start_mem(); }
    inline void destroy() {
        if(data) { 
            free(start_mem());
            data = nullptr; 
        }
    }
    inline void reserve(size_t cap) { 
        size_t* mem;
        cap = cap < SEQ_MIN_CAP ? SEQ_MIN_CAP : cap;
        size_t new_size = reserve_size(cap);
        if(data) {
            size_t* alloc_mem = start_mem();
            mem = (size_t*)(realloc(alloc_mem, new_size));
            if(mem == nullptr) {
                mem = (size_t*)(calloc(new_size, 1));
                if(data) {
                    memcpy(data, mem + 1, this->len);
                    // TODO mocos_free(data);?
                    free(alloc_mem);
                }
            } else {
                T* new_values = (T*)(mem + 1) + this->len;
                size_t c = cap - this->len - 1;
                if(c > 0) {
                    memset(new_values, 0, c * sizeof(T));
                }
            }
        } else {
            mem = (size_t*)calloc(new_size, 1);
        }
        data = (T*)(mem + 1);
        _capacity() = cap;
    }
    inline void resize(size_t newLen) { 
        if(!data || capacity() < newLen) {
            reserve(newLen);
        }
        len = newLen;
    }
    inline void push_back(T value) { 
        if(!data) {
            reserve(SEQ_MIN_CAP);
        }

        size_t& c = _capacity();
        if(this->len >= c) {
            c = c > 0 ? c * SEQ_GROWTH_FACTOR : SEQ_MIN_CAP;
            reserve(c);
        }

        data[this->len++] = value;
    }
    inline T& push() {
        if(!data) {
            reserve(64);
        }

        size_t& c = _capacity();
        if(this->len >= c) {
            c = c > 0 ? c * SEQ_GROWTH_FACTOR : SEQ_MIN_CAP;
            reserve(c);
        }

        data[this->len++] = T{};
        return data[this->len - 1];
    }
    inline size_t size() const { return len; }
    inline bool empty() const { return len == 0; }
    inline T& operator[](size_t idx) { return data[idx]; }
    inline const T& operator[](size_t idx) const { return data[idx]; }
    inline T& back() { return data[len-1]; }
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

#define s8_lit(s) (Str8{(char *)&s[0], lengthof(s)})
#define s8_str(s) (Str8{(char *)&s[0], (size_t)s.size()})
#define s8_cstr(s) (Str8{(char *)&s[0], (size_t)strlen(s)})

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


#ifdef CXB_IMPL
#include "cxb.cpp"
#endif
