/*
# cxb: Base library for CX (Orthodox-C++)

This library is my own style (Miguel's) of writing C++

## Inspiration
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
    * `StringSlice`: a pointer to char* (`data`), a length (`len`), and a flag (`null_term`) to indicate if the string
is null-terminated
        - `StringSlice` is a POD type, it does not own the memory it points to, it is only a view into a contiguous
block of memory
        - In C-land: free functions are provided, such as `cxb_ss_destroy`,
          `cxb_ss_c_str`, `cxb_ss_empty`, `cxb_ss_n_bytes`, etc.
        - In C++-land: there are methods the same operations the C free functions
          support plus operator overloads for convenience, e.g. `operator[]`,
          `slice`, `c_str`, `empty`, `size`, `n_bytes`, etc.
    * `MString`: a manually memory managed string, "M" stands for "manual"
        - This type is a `std::string` alternative, but requires manual memory management
        - Think of this type as a `StringSlice` that is optionally attached to an allocator
        - Compatible with `StringSlice`
        - In C-land: free functions are provided, such as `cxb_mstring_destroy`,
          `cxb_mstring_c_str`, `cxb_mstring_empty`, `cxb_mstring_n_bytes`, etc.
        - In C++-land: there are methods the same operations the C free functions
          support plus operator overloads for convenience, e.g. `operator[]`, `c_str`, `empty`, `size`, `n_bytes`, etc.
        - Call `.destroy()` (in C++) or `cxb_mstring_destroy` (in C) to free the memory
        - Destructor does not call `.destroy()`, see `String` for this functionality
* C++ only types: use these types when when defining C++ APIs or in implementation files
    * `String: an automatically managed string using RAII, compatible with MString
        - This type is a `std::string` alternative, with RAII semantics but requires manual copies
        - This type is an extension of `MString`, which automatically calls `destroy()` on destruction
        - This type is compatible with `StringSlice`
        - Copies must be done maually via `copy()`, i.e. the copy contructor and assignement operator are deleted
        - Moves are supported
        - Call `.release()` to remove ownership of the memory, i.e. such that the destructor does not call `destroy()`
    * `ArraySlice<T>`: similar to StringSlice but for any type
    * `MSeq<T>`: a manually memory managed expandable sequence of elements where elements are stored contiguously in
memory, "M" stands for "manual"
        - Provides an interface similar to `MString`, but for any type; "null terminated" is not supported
        - This type is a `std::vector<T>` alternative, but requires manual memory management
        - This type is compatible with `ArraySlice<T>`
        - Call `.destroy()` to free memory
        - Destructor does not call `.destroy()`, see `Seq<T>` for this functionality
    * `Seq<T>`: an automatically managed sequence container using RAII, compatible with `MSeq<T>`
        - Provides an interface similar to `String`, but for any type; "null terminated" is not supported
        - This type is a `std::vector<T>` alternative, with RAII semantics but requires manual copies
        - This type is an extension of `MSeq<T>`, which automatically calls `destroy()` on destruction
        - This type is compatible with `ArraySlice<T>`
        - Copies must be done maually via `copy()`, i.e. the copy contructor and assignement operator are deleted
        - Moves are supported
        - Call `.release()` to remove ownership of the memory, i.e. such that the destructor does not call `destroy()`
*/

#pragma once
#ifndef __cplusplus
#error "Include <cxb/cxb-c.h> when compiling C code. <cxb/cxb.h> is C++-only."
#endif

#define CXB_SKIP_C_TYPES
#define CXB_SKIP_C_PROCS
#include "cxb-c.h"
#undef CXB_SKIP_C_TYPES
#undef CXB_SKIP_C_PROCS

/* SECTION: configuration */
// #define CXB_ALLOC_TEMPLATE
// #define CXB_NAMESPACE
#define CXB_MALLOCATOR_MIN_CAP 32
#define CXB_MALLOCATOR_GROW_FN(x) (x) + (x) / 2 /* 3/2 without overflow */

#ifdef CXB_ALLOC_TEMPLATE
#error "CXB_ALLOC_TEMPLATE unsupported"
#endif

/* SECTION: includes */
#include <new>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <type_traits> // 27ms

/* SECTION: macros */
#if defined(__GNUC__)
#define CXB_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
#define CXB_INLINE __forceinline
#else
#define CXB_INLINE inline
#endif

#if defined(CXB_NAMESPACE) && defined(__cplusplus)
#define CXB_NS_BEGIN namespace cxb {
#define CXB_NS_END }
#define CXB_USE_NS using namespace cxb
#else
#define CXB_NS_BEGIN
#define CXB_NS_END
#define CXB_USE_NS
#endif

#define CXB_COMPTIME constexpr CXB_INLINE
#define CXB_COMPTIME_INLINE constexpr CXB_INLINE

#if defined(__clang_major__) && __clang_major__ >= 6
#define CXB_PURE CXB_COMPTIME __attribute__((__pure__))
#elif defined(__GNUC__) && __GNUC__ >= 6
#define CXB_PURE CXB_COMPTIME __attribute__((__pure__))
#else
#define CXB_PURE CXB_COMPTIME
#endif

CXB_NS_BEGIN

template <typename T>
static inline const T& min(const T& a, const T& b) {
    return a < b ? a : b;
}

template <typename T>
static inline const T& max(const T& a, const T& b) {
    return a > b ? a : b;
}

/* NOTE: #include <utility>  // 98ms */
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
    REQUIRES(a < b);
    return max(min(b, x), a);
}

template <typename T>
CXB_COMPTIME_INLINE void swap(T& t1, T& t2) noexcept {
    T temp(move(t1));
    t1 = move(t2);
    t2 = move(temp);
}

CXB_NS_END

/* SECTION: C API compatible types */
struct Allocator {
    size_t (*growth_sug_impl)(const struct Allocator* a, size_t count);
    void* (*alloc_impl)(
        struct Allocator* a, bool fill_zeros, void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes);
    void (*free_impl)(struct Allocator* a, void* head, size_t n_bytes);

    template <typename T, typename H>
    struct AllocationWithHeader {
        T* data;
        H* header;
    };

    // suggested growth
    CXB_MAYBE_INLINE size_t growth_sug(size_t count) const {
        return this->growth_sug_impl(this, count);
    }
    CXB_MAYBE_INLINE size_t min_count_sug() const {
        return this->growth_sug_impl(this, 0);
    }

    template <typename T, typename... Args>
    CXB_MAYBE_INLINE T* alloc(size_t count = 1, Args&&... args) {
        T* result = (T*) this->alloc_impl(this, false, nullptr, sizeof(T) * count, alignof(T), 0);
        for(size_t i = 0; i < count; ++i) new(result + i) T{args...};
        return result;
    }

    template <typename T>
    CXB_MAYBE_INLINE T* calloc(size_t old_count, size_t count = 1) {
        T* result = (T*) this->alloc_impl(this, true, nullptr, sizeof(T) * count, alignof(T), sizeof(T) * old_count);
        return result;
    }

    template <typename T, typename... Args>
    CXB_MAYBE_INLINE T* realloc(T* head, size_t old_count, size_t count = 1, Args&&... args) {
        T* result =
            (T*) this->alloc_impl(this, false, (void*) head, sizeof(T) * count, alignof(T), sizeof(T) * old_count);
        for(size_t i = old_count; i < count; ++i) {
            new(result + i) T{forward<Args>(args)...};
        }
        return result;
    }

    template <typename H, typename T, typename... Args>
    CXB_MAYBE_INLINE AllocationWithHeader<T, H> realloc_with_header(H* header,
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

    template <typename H, typename T>
    CXB_MAYBE_INLINE AllocationWithHeader<T, H> recalloc_with_header(H* header, size_t old_count, size_t count) {
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

    template <typename T>
    CXB_MAYBE_INLINE void free(T* head, size_t count) {
        this->free_impl(this, (void*) head, sizeof(T) * count);
    }

    // TODO: inconsistent
    template <typename H, typename T>
    CXB_MAYBE_INLINE void free_header_offset(T* offset_from_header, size_t count) {
        this->free_impl(this, (char*) (offset_from_header) - sizeof(H), sizeof(T) * count + sizeof(H));
    }
};

CXB_NS_BEGIN

template <typename T>
struct Atomic {
    static_assert(std::is_integral_v<T> || std::is_pointer_v<T>,
                  "Atomic wrapper only supports integral and pointer types");

    _Atomic(T) value;

    CXB_COMPTIME Atomic(T desired = T{}) noexcept : value(desired) {}

    CXB_COMPTIME Atomic(_Atomic(T) desired) noexcept : value(desired) {}

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

    // Arithmetic operations (only for integral types)
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

CXB_NS_END

struct Mallocator : Allocator {
    Mallocator();

    Atomic<i64> n_active_bytes;
    Atomic<i64> n_allocated_bytes;
    Atomic<i64> n_freed_bytes;
};

struct StringSlice {
    char* data;
    union {
        struct {
            size_t len : 62;
            bool null_term : 1;
        };
        size_t metadata;
    };

    // ** SECTION: slice compatible methods
    CXB_MAYBE_INLINE size_t n_bytes() const {
        return len + null_term;
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
    CXB_MAYBE_INLINE StringSlice slice(size_t i, size_t j = SIZE_MAX) {
        StringSlice c = *this;
        c.data = c.data + i;
        size_t new_len = j == SIZE_MAX ? len - i : j - i;
        c.len = new_len;
        c.null_term = i + new_len == len ? this->null_term : false;
        return c;
    }

    CXB_MAYBE_INLINE const char* c_str() const {
        if(!null_term) {
            return nullptr;
        }
        return data;
    }

    CXB_MAYBE_INLINE bool operator==(const StringSlice& o) const {
        if(size() != o.size()) return false;
        if(size() == 0) return true;
        return memcmp(data, o.data, size()) == 0;
    }

    CXB_MAYBE_INLINE bool operator!=(const StringSlice& o) const {
        return !(*this == o);
    }

    CXB_MAYBE_INLINE bool operator<(const StringSlice& o) const {
        size_t n = len < o.len ? len : o.len;
        int cmp = memcmp(data, o.data, n);
        if(cmp < 0) return true;
        if(cmp > 0) return false;
        return len < o.len;
    }

    CXB_MAYBE_INLINE bool operator>(const StringSlice& o) const {
        return o < *this;
    }
};

struct MString {
    char* data;
    union {
        struct {
            size_t len : 62;
            bool null_term : 1;
        };
        size_t metadata;
    };
    Allocator* allocator;

    // ** SECTION: slice compatible methods
    CXB_MAYBE_INLINE size_t n_bytes() const {
        return len + null_term;
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
    CXB_MAYBE_INLINE operator StringSlice() const {
        return StringSlice{.data = data, .len = len, .null_term = null_term};
    }

    CXB_MAYBE_INLINE StringSlice slice(size_t i, size_t j = SIZE_MAX) {
        StringSlice c = *this;
        c.data = c.data + i;
        size_t new_len = j == SIZE_MAX ? len - i : j - i;
        c.len = new_len;
        c.null_term = i + new_len == len ? this->null_term : false;
        return c;
    }
    CXB_MAYBE_INLINE const char* c_str() const {
        if(!null_term) {
            return nullptr;
        }
        return data;
    }

    CXB_MAYBE_INLINE bool operator==(const StringSlice& o) const {
        if(size() != o.size()) return false;
        if(size() == 0) return true;
        return memcmp(data, o.data, size()) == 0;
    }

    CXB_MAYBE_INLINE bool operator!=(const StringSlice& o) const {
        return !(*this == o);
    }

    CXB_MAYBE_INLINE bool operator<(const StringSlice& o) const {
        size_t n = len < o.len ? len : o.len;
        int cmp = memcmp(data, o.data, n);
        if(cmp < 0) return true;
        if(cmp > 0) return false;
        return len < o.len;
    }

    CXB_MAYBE_INLINE bool operator>(const StringSlice& o) const {
        return o < *this;
    }

    // ** SECTION: allocator-related methods
    CXB_MAYBE_INLINE MString& copy_(Allocator* to_allocator = &default_alloc) {
        *this = move(this->copy(to_allocator));
        return *this;
    }

    CXB_MAYBE_INLINE MString copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        REQUIRES(to_allocator != nullptr);

        MString result{.data = data, .len = len, .null_term = null_term, .allocator = to_allocator};
        return result;
    }

    CXB_MAYBE_INLINE const char* c_str_maybe_copy(Allocator* copy_alloc_if_not) {
        if(!null_term) {
            ensure_null_terminated(copy_alloc_if_not);
        }
        return data;
    }

    CXB_MAYBE_INLINE size_t* start_mem() {
        if(this->data == nullptr) return nullptr;
        return ((size_t*) this->data) - 1;
    }

    CXB_MAYBE_INLINE size_t& _capacity() {
        REQUIRES(allocator);
        return *start_mem();
    }

    CXB_MAYBE_INLINE size_t capacity() {
        if(!data) return 0;
        return *start_mem();
    }

    CXB_MAYBE_INLINE void destroy() {
        if(data && allocator) {
            allocator->free_header_offset<size_t>(data, capacity());
            data = nullptr;
        }
    }

    CXB_MAYBE_INLINE void ensure_capacity(size_t new_capacity = 0) {
        if(new_capacity == 0) {
            new_capacity = len + null_term;
        }

        if(capacity() < new_capacity) {
            reserve(new_capacity);
        }
    }

    void reserve(size_t cap) {
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
            if(null_term) {
                data[len + 1] = '\0';
            }
            len += 1;
        }
    }

    CXB_MAYBE_INLINE char& push() {
        push_back('\0');
        return data[len - 1];
    }

    CXB_MAYBE_INLINE char pop_back() {
        REQUIRES(len > 0);
        char ret = data[len - 1];
        if(null_term && len > 0) {
            data[len - 1] = '\0';
        }
        len--;
        return ret;
    }

    void extend(StringSlice other) {
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

    CXB_MAYBE_INLINE void operator+=(StringSlice other) {
        this->extend(other);
    }

    CXB_MAYBE_INLINE void extend(const char* str, size_t n = SIZE_MAX) {
        if(!str) {
            return;
        }
        size_t len = n == SIZE_MAX ? strlen(str) : n;
        this->extend(StringSlice{.data = const_cast<char*>(str), .len = len, .null_term = true});
    }

    CXB_MAYBE_INLINE void ensure_null_terminated(Allocator* copy_alloc_if_not = nullptr) {
        if(null_term) return;

        REQUIRES(allocator != nullptr || copy_alloc_if_not != nullptr);
        if(allocator == nullptr) {
            *this = move(this->copy(copy_alloc_if_not));
        } else {
            this->push_back('\0');
            this->null_term = true;
        }
    }
};

/* SECTION: C++-only API */
CXB_NS_BEGIN

// TODO: placement new?
// inline void *operator new(size_t, void *p) noexcept { return p; }

/* SECTION: containers */
struct String : MString {
    String(Allocator* allocator = &default_alloc)
        : MString{.data = nullptr, .len = 0, .null_term = true, .allocator = allocator} {}

    String(const MString& m)
        : MString{.data = m.data, .len = m.len, .null_term = m.null_term, .allocator = m.allocator} {}

    String(const char* cstr, size_t n = SIZE_MAX, bool null_term = true, Allocator* allocator = &default_alloc)
        : MString{.data = nullptr,
                  .len = n == SIZE_MAX ? strlen(cstr) : n,
                  .null_term = null_term,
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

    String(const String& o) : MString{.data = nullptr, .len = 0, .null_term = true, .allocator = nullptr} {
        data = o.data;
        metadata = o.metadata;
    }

    String(String&& o) : MString{.data = nullptr, .len = 0, .null_term = true, .allocator = o.allocator} {
        data = o.data;
        metadata = o.metadata;
        o.allocator = nullptr;
    }

    String& operator=(const String& o) = delete;
    CXB_INLINE String& operator=(String&& o) {
        allocator = o.allocator;
        o.allocator = nullptr;
        data = o.data;
        metadata = o.metadata;
        return *this;
    }

    CXB_MAYBE_INLINE ~String() {
        destroy();
    }

    CXB_MAYBE_INLINE String& copy_(Allocator* to_allocator = &default_alloc) {
        *this = move(this->copy(to_allocator));
        return *this;
    }

    CXB_MAYBE_INLINE String copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        REQUIRES(to_allocator != nullptr);

        String result{data, len, null_term, to_allocator};
        return result;
    }

    CXB_MAYBE_INLINE MString release() {
        MString result{.data = data, .len = len, .null_term = null_term, .allocator = allocator};
        this->allocator = nullptr;
        return result;
    }
};

template <typename T>
struct Seq {
    T* data;
    size_t len;
    Allocator* allocator;

    Seq(Allocator* allocator = &default_alloc) : data{nullptr}, len{0}, allocator{allocator} {
        if(allocator) {
            reserve(0);
        }
    }
    Seq(T* data, size_t n, Allocator* allocator = &default_alloc) : data{data}, len{n}, allocator{allocator} {
        if(allocator) {
            reserve(0);
        }
    }
    Seq(Seq<T>&& o) : data{o.data}, len{o.len}, allocator{o.allocator} {
        o.allocator = nullptr;
    }
    Seq<T>& operator=(Seq<T>&& o) {
        allocator = o.allocator;
        o.allocator = nullptr;
        data = o.data;
        len = o.len;
        return *this;
    }

    Seq(const Seq<T>& o) = delete;
    Seq<T>& operator=(const Seq<T>& o) = delete;

    ~Seq() {
        destroy();
    }

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
    CXB_INLINE Seq<T> slice(size_t i = 0, size_t j = SIZE_MAX) {
        return Seq<T>{data + i, j == SIZE_MAX ? len : j - i, nullptr};
    }

    CXB_INLINE bool operator<(const Seq<T>& o) const {
        size_t n = len < o.len ? len : o.len;
        for(size_t i = 0; i < n; ++i) {
            if(data[i] < o.data[i]) return true;
            if(o.data[i] < data[i]) return false;
        }
        return len < o.len;
    }

    CXB_INLINE bool operator==(const Seq<T>& o) const {
        if(len != o.len) return false;
        for(size_t i = 0; i < len; ++i) {
            if(!(data[i] == o.data[i])) return false;
        }
        return true;
    }

    CXB_INLINE bool operator!=(const Seq<T>& o) const {
        return !(*this == o);
    }

    CXB_INLINE bool operator>(const Seq<T>& o) const {
        return o < *this;
    }

    // ** SECTION: allocator-related methods
    CXB_INLINE Seq<T>& copy_(Allocator* to_allocator = &default_alloc) {
        *this = move(this->copy(to_allocator));
        return *this;
    }

    CXB_MAYBE_INLINE Seq<T> copy(Allocator* to_allocator = nullptr) {
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
            for(size_t i = 0; i < len; ++i) {
                data[i].~T();
            }
            allocator->free_header_offset<size_t>(data, capacity());
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

    template <typename... Args>
    CXB_MAYBE_INLINE void resize(size_t new_len, Args&&... args) {
        if(capacity() < new_len) {
            reserve(new_len);
        }
        for(size_t i = len; i < new_len; ++i) {
            // TODO: new []
            new(data + i) T{forward<Args>(args)...};
        }
        len = new_len;
    }

    CXB_MAYBE_INLINE void push_back(T value) {
        REQUIRES(UNLIKELY(allocator != nullptr));
        size_t c = capacity();
        if(this->len >= c) {
            c = allocator->growth_sug(c);
            reserve(c);
        }

        data[this->len++] = move(value);
    }

    CXB_MAYBE_INLINE T& push() {
        size_t c = capacity();
        if(this->len >= c) {
            c = allocator->growth_sug(c);
            reserve(c);
        }

        data[this->len++] = T{};
        return data[this->len - 1];
    }

    CXB_MAYBE_INLINE T pop_back() {
        REQUIRES(len > 0);
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
};

/* SECTION: variant types */

template <typename T>
struct Optional {
    T value;
    bool exists;
};
CXB_NS_END

/* SECTION: C API (continued) */
// TODO: implement arena allocator
// struct Arena : Allocator {
//     Arena();
// };

#define S8_STR(s) (StringSlice{.data = (char*) s.c_str(), .len = (size_t) s.size(), .null_term = true})

#ifdef CXB_IMPL
#include "cxb.cpp"
#endif