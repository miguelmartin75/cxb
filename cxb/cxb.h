/*
# cxb: Base library for CX (Orthodox-C++)

This library is my own style (Miguel's) of writing C++. This does include RAII
by default, but it can be disabled at compile-time and run-time. Please see below
in the "configuration" section.

## Inspiration
- Zig
- Python
- Nim

## Containers
* Seq<T>
* Str8

*/

#pragma once

/* SECTION: configuration */
// #define CXB_DISABLE_RAII
// #define CXB_ALLOC_TEMPLATE
// #define CXB_NAMESPACE
// #define CXB_USE_C11_ATOMIC /* note: C11 stdatomic.h takes less time to compile */
#define CXB_MALLOCATOR_MIN_CAP 32
#define CXB_MALLOCATOR_GROW_FN(x) (x) + (x) / 2 /* 3/2 without overflow */

#ifdef CXB_ALLOC_TEMPLATE
#error "CXB_ALLOC_TEMPLATE unsupported"
#endif

/* SECTION: includes */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // TODO: removeme
#include <string.h>
#include <type_traits> // 27ms
#include <new>

#ifdef CXB_USE_C11_ATOMIC
// NOTE: GCC doesn't support _Atomic in C++
#if defined(__STDC_NO_ATOMICS__) || (defined(__GNUC__) && !defined(__clang__))
#warning "Using std::atomic as C11 _Atomic is not available"
#undef CXB_USE_C11_ATOMIC
#endif
#endif

#ifdef CXB_USE_C11_ATOMIC
extern "C" {
#include <stdatomic.h>
}
#else
#include <atomic> // 114-128ms
#endif

/* NOTE: #include <utility>  // 98ms */

/* SECTION: macros */
// NOTE some macros are copied/modified from Blend2D, see:
// https://github.com/blend2d/blend2d/blob/bae3b6c600186a69a9f212243ed9700dc93a314a/src/blend2d/api.h#L563
#define CXB_EXPORT
#define CXB_INTERNAL static

#ifdef CXB_NAMESPACE
#define CXB_NS_BEGIN namespace cxb {
#define CXB_NS_END }
#define CXB_USE_NS using namespace cxb
#else
#define CXB_NS_BEGIN
#define CXB_NS_END
#define CXB_USE_NS
#endif

#if defined(__clang__)
#define BREAKPOINT() __builtin_debugtrap()
#elif defined(__GNUC__)
#define BREAKPOINT() __builtin_trap()
#else
#define BREAKPOINT() abort()
#endif

#ifdef __cplusplus
#define CXB_C_EXPORT extern "C"
#define CXB_C_IMPORT extern "C"
#else
#define CXB_C_EXPORT
#define CXB_C_IMPORT
#endif

#define COUNTOF_LIT(a) (size_t) (sizeof(a) / sizeof(*(a)))
#define LENGTHOF_LIT(s) (COUNTOF_LIT(s) - 1)
#define ASSERT(x, msg)    \
    if(!(x)) BREAKPOINT()
#define REQUIRES(x)       \
    if(!(x)) BREAKPOINT()
#define LIKELY(x) x
#define UNLIKELY(x) x
// #define INFO(msg)
// #define WARN(msg)
// #define FATAL(msg)

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

#define CXB_COMPTIME constexpr CXB_INLINE_NODEBUG
#define CXB_COMPTIME_INLINE constexpr CXB_INLINE

#if defined(__clang_major__) && __clang_major__ >= 6
#define CXB_PURE CXB_COMPTIME __attribute__((__pure__))
#elif defined(__GNUC__) && __GNUC__ >= 6
#define CXB_PURE CXB_COMPTIME __attribute__((__pure__))
#else
#define CXB_PURE CXB_COMPTIME
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

#if defined(__GNUC__)
typedef __uint128_t u128;
typedef __int128_t i128;
#else
// TODO: support MSVC, etc.
#endif

enum class MemoryOrderOption { Relaxed, Acquire, Release, AcqRel, SeqCst };

template <typename T>
class Atomic {
    static_assert(std::is_integral_v<T> || std::is_pointer_v<T>,
                  "AtomicWrapper only supports integral and pointer types");

  private:
#ifdef CXB_USE_C11_ATOMIC
    _Atomic T value;
#else
    std::atomic<T> value;
#endif

  public:
    CXB_COMPTIME Atomic(T desired = T{}) noexcept
#ifdef CXB_USE_C11_ATOMIC
        : value(desired)
#else
        : value(desired)
#endif
    {
    }

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;
    Atomic(Atomic&&) = delete;
    Atomic& operator=(Atomic&&) = delete;

    CXB_INLINE void store(T desired, MemoryOrderOption order = MemoryOrderOption::SeqCst) noexcept {
#ifdef CXB_USE_C11_ATOMIC
        atomic_store_explicit(&value, desired, to_c11_order(order));
#else
        value.store(desired, to_std_order(order));
#endif
    }

    CXB_INLINE T load(MemoryOrderOption order = MemoryOrderOption::SeqCst) const noexcept {
#ifdef CXB_USE_C11_ATOMIC
        return atomic_load_explicit(&value, to_c11_order(order));
#else
        return value.load(to_std_order(order));
#endif
    }

    CXB_INLINE T exchange(T desired, MemoryOrderOption order = MemoryOrderOption::SeqCst) noexcept {
#ifdef CXB_USE_C11_ATOMIC
        return atomic_exchange_explicit(&value, desired, to_c11_order(order));
#else
        return value.exchange(desired, to_std_order(order));
#endif
    }

    CXB_INLINE bool compare_exchange_weak(T& expected,
                                          T desired,
                                          MemoryOrderOption success = MemoryOrderOption::SeqCst,
                                          MemoryOrderOption failure = MemoryOrderOption::SeqCst) noexcept {
#ifdef CXB_USE_C11_ATOMIC
        return atomic_compare_exchange_weak_explicit(
            &value, &expected, desired, to_c11_order(success), to_c11_order(failure));
#else
        return value.compare_exchange_weak(expected, desired, to_std_order(success), to_std_order(failure));
#endif
    }

    CXB_INLINE bool compare_exchange_strong(T& expected,
                                            T desired,
                                            MemoryOrderOption success = MemoryOrderOption::SeqCst,
                                            MemoryOrderOption failure = MemoryOrderOption::SeqCst) noexcept {
#ifdef CXB_USE_C11_ATOMIC
        return atomic_compare_exchange_strong_explicit(
            &value, &expected, desired, to_c11_order(success), to_c11_order(failure));
#else
        return value.compare_exchange_strong(expected, desired, to_std_order(success), to_std_order(failure));
#endif
    }

    // Arithmetic operations (only for integral types)
    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> fetch_add(
        T arg, MemoryOrderOption order = MemoryOrderOption::SeqCst) noexcept {
#ifdef CXB_USE_C11_ATOMIC
        return atomic_fetch_add_explicit(&value, arg, to_c11_order(order));
#else
        return value.fetch_add(arg, to_std_order(order));
#endif
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> fetch_sub(
        T arg, MemoryOrderOption order = MemoryOrderOption::SeqCst) noexcept {
#ifdef CXB_USE_C11_ATOMIC
        return atomic_fetch_sub_explicit(&value, arg, to_c11_order(order));
#else
        return value.fetch_sub(arg, to_std_order(order));
#endif
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> fetch_and(
        T arg, MemoryOrderOption order = MemoryOrderOption::SeqCst) noexcept {
#ifdef CXB_USE_C11_ATOMIC
        return atomic_fetch_and_explicit(&value, arg, to_c11_order(order));
#else
        return value.fetch_and(arg, to_std_order(order));
#endif
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> fetch_or(
        T arg, MemoryOrderOption order = MemoryOrderOption::SeqCst) noexcept {
#ifdef CXB_USE_C11_ATOMIC
        return atomic_fetch_or_explicit(&value, arg, to_c11_order(order));
#else
        return value.fetch_or(arg, to_std_order(order));
#endif
    }

    template <typename U = T>
    CXB_INLINE typename std::enable_if_t<std::is_integral_v<U>, T> fetch_xor(
        T arg, MemoryOrderOption order = MemoryOrderOption::SeqCst) noexcept {
#ifdef CXB_USE_C11_ATOMIC
        return atomic_fetch_xor_explicit(&value, arg, to_c11_order(order));
#else
        return value.fetch_xor(arg, to_std_order(order));
#endif
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
#ifdef CXB_USE_C11_ATOMIC
        return atomic_is_lock_free(&value);
#else
        return value.is_lock_free();
#endif
    }

    static constexpr bool is_always_lock_free =
#ifdef CXB_USE_C11_ATOMIC
        true; // C11 doesn't have a compile-time check
#else
        std::atomic<T>::is_always_lock_free;
#endif

#ifdef CXB_USE_C11_ATOMIC
    static CXB_COMPTIME_INLINE memory_order to_c11_order(MemoryOrderOption order) {
        switch(order) {
            case MemoryOrderOption::Relaxed:
                return memory_order_relaxed;
            case MemoryOrderOption::Acquire:
                return memory_order_acquire;
            case MemoryOrderOption::Release:
                return memory_order_release;
            case MemoryOrderOption::AcqRel:
                return memory_order_acq_rel;
            case MemoryOrderOption::SeqCst:
                return memory_order_seq_cst;
        }
        return memory_order_seq_cst;
    }
#else
    static CXB_COMPTIME_INLINE std::memory_order to_std_order(MemoryOrderOption order) {
        switch(order) {
            case MemoryOrderOption::Relaxed:
                return std::memory_order_relaxed;
            case MemoryOrderOption::Acquire:
                return std::memory_order_acquire;
            case MemoryOrderOption::Release:
                return std::memory_order_release;
            case MemoryOrderOption::AcqRel:
                return std::memory_order_acq_rel;
            case MemoryOrderOption::SeqCst:
                return std::memory_order_seq_cst;
        }
        return std::memory_order_seq_cst;
    }
#endif
};

/* SECTION: primitive functions */
template <class T>
CXB_PURE const T& min(const T& a, const T& b) {
    return a < b ? a : b;
}
template <class T>
CXB_PURE const T& max(const T& a, const T& b) {
    return a > b ? a : b;
}
template <class T>
CXB_PURE const T& clamp(const T& x, const T& a, const T& b) {
    REQUIRES(a < b);
    return max(min(b, x), a);
}

// NOTE: move/forward/swap are copied from Blend2D
template <typename T>
CXB_PURE typename std::remove_reference<T>::type&& move(T&& v) noexcept {
    return static_cast<typename std::remove_reference<T>::type&&>(v);
}

template <typename T>
CXB_PURE T&& forward(typename std::remove_reference<T>::type& v) noexcept {
    return static_cast<T&&>(v);
}

template <typename T>
CXB_PURE T&& forward(typename std::remove_reference<T>::type&& v) noexcept {
    return static_cast<T&&>(v);
}

template <typename T>
CXB_COMPTIME_INLINE void swap(T& t1, T& t2) noexcept {
    T temp(move(t1));
    t1 = move(t2);
    t2 = move(temp);
}

// TODO: placement new?
// inline void *operator new(size_t, void *p) noexcept { return p; }

struct Allocator {
    size_t (*growth_sug_impl)(const Allocator* a, size_t count);
    void* (*alloc_impl)(
        Allocator* a, bool fill_zeros, void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes);
    void (*free_impl)(Allocator* a, void* head, size_t n_bytes);

    template <class T, class H>
    struct AllocationWithHeader {
        T* data;
        H* header;
    };

    // suggested growth
    CXB_INLINE size_t growth_sug(size_t count) const {
        return this->growth_sug_impl(this, count);
    }
    CXB_INLINE size_t min_count_sug() const {
        return this->growth_sug_impl(this, 0);
    }

    template <class T, class... Args>
    CXB_INLINE T* alloc(size_t count = 1, Args&&... args) {
        T* result = (T*) this->alloc_impl(this, false, nullptr, sizeof(T) * count, alignof(T), 0);
        for(size_t i = 0; i < count; ++i) new(result + i) T{args...};
        return result;
    }

    template <class T>
    CXB_INLINE T* calloc(size_t old_count, size_t count = 1) {
        T* result = (T*) this->alloc_impl(this, true, nullptr, sizeof(T) * count, alignof(T), sizeof(T) * old_count);
        return result;
    }

    template <class T, class... Args>
    CXB_INLINE T* realloc(T* head, size_t old_count, size_t count = 1, Args&&... args) {
        T* result =
            (T*) this->alloc_impl(this, false, (void*) head, sizeof(T) * count, alignof(T), sizeof(T) * old_count);
        for(size_t i = old_count; i < count; ++i) {
            new(result + i) T{forward<Args>(args)...};
        }
        return result;
    }

    template <class H, class T, class... Args>
    CXB_INLINE AllocationWithHeader<T, H> realloc_with_header(H* header,
                                                              size_t old_count,
                                                              size_t count,
                                                              Args&&... args) {
        char* new_header = (char*) this->alloc_impl(this,                             // allocator
                                                    false,                            // fill_zeros
                                                    (void*) header,                   // header
                                                    sizeof(T) * count + sizeof(H),    // n_bytes
                                                    alignof(T) + sizeof(H),           // alignment
                                                    sizeof(T) * old_count + sizeof(H) // old bytes
        );
        T* data = (T*) (new_header + sizeof(H));

        for(size_t i = old_count; i < count; ++i) {
            new(data + i) T{forward<Args>(args)...};
        }
        return AllocationWithHeader<T, H>{data, (H*) new_header};
    }

    template <class H, class T>
    CXB_INLINE AllocationWithHeader<T, H> recalloc_with_header(H* header, size_t old_count, size_t count) {
        char* new_header = (char*) this->alloc_impl(this,                                               // allocator
                                                    true,                                               // fill_zeros
                                                    (void*) header,                                     // header
                                                    sizeof(T) * count + sizeof(H),                      // n_bytes
                                                    alignof(T) + sizeof(H),                             // alignment
                                                    sizeof(T) * old_count + sizeof(H) * (old_count > 0) // old bytes
        );
        T* data = (T*) (new_header + sizeof(H));
        return AllocationWithHeader<T, H>{data, (H*) new_header};
    }

    template <class T>
    CXB_INLINE void free(T* head, size_t count) {
        this->free_impl(this, (void*) head, sizeof(T) * count);
    }

    // TODO: inconsistent
    template <class H, class T>
    CXB_INLINE void free_with_header(T* head, size_t count) {
        this->free_impl(this, (char*) (head) - sizeof(H), sizeof(T) * count + sizeof(H));
    }
};

struct Mallocator : Allocator {
    Mallocator();
    Atomic<i64> n_active_bytes;
    Atomic<i64> n_allocated_bytes;
    Atomic<i64> n_freed_bytes;
};

struct Arena : Allocator {
    Arena();
};

extern Mallocator default_alloc;

/* SECTION: containers */

/* SUB-SECTION: strings */
struct Str8 {
    char* data;
    union {
        struct {
            size_t len : 62;
            bool null_term : 1;
        };
        size_t metadata;
    };

#ifdef __cplusplus
    // ** SECTION: slice compatible methods
    CXB_INLINE size_t size() const {
        return len;
    }
    CXB_INLINE bool empty() const {
        return len == 0;
    }
    CXB_INLINE char& operator[](size_t idx) {
        return data[idx];
    }
    CXB_INLINE const char& operator[](size_t idx) const {
        return data[idx];
    }
    CXB_INLINE char& back() {
        return data[len - 1];
    }
    CXB_INLINE Str8 slice(size_t i, size_t j = 0) {
        Str8 c = *this;
        c.data = c.data + i;
        size_t new_len = j == 0 ? len - i : j - i;
        c.len = new_len;
        c.null_term = i + new_len == len ? this->null_term : false;
        return c;
    }
    CXB_INLINE const char* c_str() const {
        if(!null_term) {
            return nullptr;
        }
        return data;
    }

    CXB_INLINE bool operator==(const Str8& o) const {
        if(size() != o.size()) return false;
        for(size_t i = 0; i < size(); ++i) {
            if((*this)[i] != o[i]) return false;
        }
        return true;
    }

    CXB_INLINE bool operator!=(const Str8& o) const {
        return !(*this == o);
    }

    CXB_INLINE bool operator<(const Str8& o) const {
        for(size_t i = 0; i < min(len, o.len); ++i) {
            if((*this)[i] >= o[i]) return false;
        }
        return len < o.len;
    }
#endif
};

struct String: Str8 {
    Allocator* allocator;

    String(Allocator* allocator = &default_alloc) : Str8{.data = nullptr, .len = 0, .null_term = true}, allocator{allocator} {
        reserve(0);
    }

    String(const char* cstr, size_t n = SIZE_MAX, Allocator* allocator = &default_alloc)
        : Str8{nullptr, n == SIZE_MAX ? strlen(cstr) : n, true}, allocator{allocator} {
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
    String(const String& o) : Str8{.data = nullptr, .len = 0, .null_term = true}, allocator{nullptr} {
        data = o.data;
        metadata = o.metadata;
    }

    String(String&& o) : Str8{.data = nullptr, .len = 0, .null_term = true}, allocator{o.allocator} {
        data = o.data;
        metadata = o.metadata;
        o.allocator = nullptr;
    }

    CXB_INLINE String& operator=(const String& o) {
        allocator = nullptr;
        data = o.data;
        metadata = o.metadata;
        return *this;
    }

    CXB_INLINE String& operator=(String&& o) {
        allocator = o.allocator;
        o.allocator = nullptr;
        data = o.data;
        metadata = o.metadata;
        return *this;
    }

#ifndef CXB_DISABLE_RAII
    CXB_INLINE ~String() {
        destroy();
    }
#endif


    // ** SECTION: allocator-related methods
    CXB_INLINE const char* c_str_maybe_copy(Allocator* copy_alloc_if_not) {
        if(!null_term) {
            ensure_null_terminated(copy_alloc_if_not);
        }
        return data;
    }

    CXB_INLINE String& copy_(Allocator* to_allocator = &default_alloc) {
        *this = move(this->copy(to_allocator));
        return *this;
    }

    CXB_INLINE String copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        REQUIRES(to_allocator != nullptr);

        String result{data, len, to_allocator};
        return result;
    }

    CXB_INLINE size_t* start_mem() {
        if(this->data == nullptr) return nullptr;
        return ((size_t*) this->data) - 1;
    }

    CXB_INLINE size_t& _capacity() {
        REQUIRES(allocator);
        return *start_mem();
    }

    CXB_INLINE size_t capacity() {
        if(!data) return 0;
        return *start_mem();
    }

    CXB_INLINE void destroy() {
        if(data && allocator) {
            allocator->free_with_header<size_t>(data, capacity());
            data = nullptr;
        }
    }

    CXB_INLINE void reserve(size_t cap) {
        REQUIRES(allocator != nullptr);

        size_t* alloc_mem = start_mem();
        size_t old_count = capacity();
        size_t new_count = max(cap, allocator->min_count_sug());
        auto mem = allocator->recalloc_with_header<size_t, char>(alloc_mem, old_count, new_count);
        data = mem.data;
        _capacity() = new_count;
    }

    void resize(size_t new_len, char fill_char = '\0') {
        bool was_null_terminated = null_term;
        size_t reserve_size = new_len + was_null_terminated;

        if(capacity() < reserve_size) {
            reserve(reserve_size);
        }

        size_t old_len = len;
        if(new_len > old_len) {
            memset(data + old_len, fill_char, new_len - old_len);
        }

        if(was_null_terminated) {
            data[new_len] = '\0';
        }

        len = new_len;
        null_term = was_null_terminated;
    }

    void push_back(char c) {
        REQUIRES(UNLIKELY(allocator != nullptr));
        size_t needed_cap = len + null_term + 1;
        if(capacity() < needed_cap) {
            size_t new_cap = allocator->growth_sug(capacity());
            if(new_cap < needed_cap) new_cap = needed_cap;
            reserve(new_cap);
        }

        data[len] = c;
        if(UNLIKELY(c == '\0')) {
            null_term = true;
        } else {
            len += 1;
            if(null_term) {
                data[len + 1] = '\0';
            }
        }
    }

    CXB_INLINE char& push() {
        push_back('\0');
        return data[len - 1];
    }

    CXB_INLINE char pop_back() {
        REQUIRES(len > 0);
        char ret = data[len - 1];
        if(null_term && len > 0) {
            data[len - 1] = '\0';
        }
        len--;
        return ret;
    }

    void extend(Str8 other) {
        if(other.len == 0) return;
        REQUIRES(allocator);

        size_t needed_cap = len + other.len;
        if(capacity() < needed_cap) {
            size_t new_cap = allocator->growth_sug(capacity());
            if(new_cap < needed_cap) new_cap = needed_cap;
            reserve(new_cap);
        }

        memcpy(data + len, other.data, other.len);
        len += other.len;
    }

    CXB_INLINE void operator+=(Str8 other) {
        this->extend(other);
    }

    CXB_INLINE void extend(const char* str, size_t n = SIZE_MAX) {
        if(!str) {
            return;
        }
        this->extend( Str8{const_cast<char*>(str), .len = n == SIZE_MAX ? strlen(str) : n, .null_term = true});
    }

    CXB_INLINE void ensure_null_terminated(Allocator* copy_alloc_if_not = nullptr) {
        if(null_term) return;

        REQUIRES(allocator != nullptr || copy_alloc_if_not != nullptr);
        if(allocator == nullptr) {
            *this = move(this->copy(copy_alloc_if_not));
        } else {
            this->push_back('\0');
            this->null_term = true;
        }
    }

    CXB_INLINE void release() {
        this->allocator = nullptr;
    }
};


#ifdef __cplusplus

template <class T> // NOTE: could use allocator as template param
struct Seq {
    T* data;
    size_t len;
    Allocator* allocator;

    Seq(Allocator* allocator = &default_alloc) : allocator{allocator}, data{nullptr}, len{0} {
        if(allocator) {
            reserve(0);
        }
    }
    Seq(T* data, size_t n, Allocator* allocator = &default_alloc) : allocator{allocator}, data{data}, len{n} {
        if(allocator) {
            reserve(0);
        }
    }
    Seq(const Seq<T>& o) : allocator{nullptr}, data{o.data}, len{o.len} {}
    Seq(Seq<T>&& o) : allocator{o.allocator}, data{o.data}, len{o.len} {
        o.allocator = nullptr;
    }
    Seq<T>& operator=(const Seq<T>& o) {
        allocator = nullptr;
        data = o.data;
        len = o.len;
        return *this;
    }
    Seq<T>& operator=(Seq<T>&& o) {
        allocator = o.allocator;
        o.allocator = nullptr;
        data = o.data;
        len = o.len;
        return *this;
    }

#ifndef CXB_DISABLE_RAII
    CXB_INLINE ~Seq() {
        destroy();
    }
#endif

    // ** SECTION: slice compatible methods
    CXB_INLINE size_t size() const {
        return len;
    }
    CXB_INLINE bool empty() const {
        return len == 0;
    }
    CXB_INLINE T& operator[](size_t idx) {
        return data[idx];
    }
    CXB_INLINE const T& operator[](size_t idx) const {
        return data[idx];
    }
    CXB_INLINE T& back() {
        return data[len - 1];
    }
    CXB_INLINE Seq<T> slice(size_t i, size_t j = 0) {
        Seq<T> c = *this;
        c.data = c.data + i;
        c.len = j == 0 ? c.len : j - i + 1;
        return c;
    }

    // ** SECTION: allocator-related methods
    CXB_INLINE Seq<T>& copy_(Allocator* to_allocator = &default_alloc) {
        *this = move(this->copy(to_allocator));
        return *this;
    }

    inline Seq<T> copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        REQUIRES(to_allocator != nullptr);
        Seq<T> result{nullptr, 0, to_allocator};
        result.reserve(len);
        if(data && len > 0) {
            memcpy(result.data, data, len * sizeof(T));
        }
        result.len = len;
        return result;
    }

    CXB_INLINE size_t* start_mem() {
        if(this->data == nullptr) return nullptr;
        return ((size_t*) this->data) - 1;
    }

    CXB_INLINE size_t& _capacity() {
        REQUIRES(data);
        return *start_mem();
    }

    CXB_INLINE size_t capacity() {
        if(!data) return 0;
        return *start_mem();
    }

    CXB_INLINE void destroy() {
        if(data && allocator) {
            allocator->free_with_header<size_t>(data, capacity());
            data = nullptr;
        }
    }

    CXB_INLINE void reserve(size_t cap) {
        REQUIRES(allocator != nullptr);

        size_t* alloc_mem = start_mem();
        size_t old_count = capacity();
        size_t new_count = max(cap, allocator->min_count_sug());
        auto mem = allocator->recalloc_with_header<size_t, T>(alloc_mem, old_count, new_count);
        data = mem.data;
        _capacity() = new_count;
    }

    template <class... Args>
    inline void resize(size_t new_len, Args&&... args) {
        if(capacity() < new_len) {
            reserve(new_len);
        }
        for(size_t i = len; i < new_len; ++i) {
            // TODO: new []
            new(data + i) T{forward<Args>(args)...};
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
#endif

/* SECTION: math types */
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

/* SECTION: variant types */

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

#define S8_LIT(s) (Str8{.data = (char*) &s[0], .len = LENGTHOF_LIT(s), .null_term = true})
#define S8_DATA(c, l) (Str8{.data = (char*) &c[0], .len = (l), .null_term = false})
#define S8_STR(s) (Str8{.data = (char*) s.c_str(), .len = (size_t) s.size(), .null_term = true})
#define S8_CSTR(s) (Str8{.data = (char*) s, .len = (size_t) strlen(s), .null_term = true})
