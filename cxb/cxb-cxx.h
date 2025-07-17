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
    * `AString: an automatically managed string using RAII, compatible with MString
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

#ifndef CXB_H
#define CXB_H

#ifndef __cplusplus
#error "Include <cxb/cxb-c.h> when compiling C code. <cxb/cxb.h> is C++-only."
#endif

#include "cxb-c.h"

// NOTE: GCC doesn't support _Atomic in C++
#if defined(__STDC_NO_ATOMICS__) || (defined(__GNUC__) && !defined(__clang__))
#error "C11 _Atomic is not available, compile with C++23"
#endif

/* SECTION: configuration */
// #define CXB_ALLOC_TEMPLATE
// #define CXB_NAMESPACE
#define CXB_SEQ_MIN_CAP 32
#define CXB_SEQ_GROW_FN(x) (x) + (x) / 2 /* 3/2 without overflow */
#define CXB_STR_MIN_CAP 32
#define CXB_STR_GROW_FN(x) (x) + (x) / 2 /* 3/2 without overflow */

#ifdef CXB_ALLOC_TEMPLATE
#error "CXB_ALLOC_TEMPLATE unsupported"
#endif

/* SECTION: includes */
#include <new>
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
    void* (*alloc_impl)(
        struct Allocator* a, bool fill_zeros, void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes);
    void (*free_impl)(struct Allocator* a, void* head, size_t n_bytes);

    template <typename T, typename H>
    struct AllocationWithHeader {
        T* data;
        H* header;
    };

    template <typename T>
    CXB_MAYBE_INLINE T* alloc(size_t count = 1) {
        T* result = (T*) this->alloc_impl(this, false, nullptr, sizeof(T) * count, alignof(T), 0);
        return result;
    }

    template <typename T>
    CXB_MAYBE_INLINE T* calloc(size_t old_count, size_t count = 1) {
        T* result = (T*) this->alloc_impl(this, true, nullptr, sizeof(T) * count, alignof(T), sizeof(T) * old_count);
        return result;
    }

    template <typename T>
    CXB_MAYBE_INLINE T* realloc(T* head, size_t old_count, bool fill_zeros, size_t count) {
        T* result =
            (T*) this->alloc_impl(this, fill_zeros, (void*) head, sizeof(T) * count, alignof(T), sizeof(T) * old_count);
        return result;
    }

    template <typename H, typename T>
    CXB_MAYBE_INLINE AllocationWithHeader<T, H> realloc_with_header(H* header, size_t old_count, size_t count) {
        char* new_header = (char*) this->alloc_impl(this,                             // allocator
                                                    false,                            // fill_zeros
                                                    (void*) header,                   // header
                                                    sizeof(T) * count + sizeof(H),    // n_bytes
                                                    alignof(T) + sizeof(H),           // alignment
                                                    sizeof(T) * old_count + sizeof(H) // old bytes
        );
        T* data = (T*) (new_header + sizeof(H));
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
            size_t len : 63;
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
    CXB_MAYBE_INLINE StringSlice slice(i64 i = 0, i64 j = -1) const {
        i64 ii = i < 0 ? len + i : i;
        i64 jj = j < 0 ? len + j : j;
        DEBUG_ASSERT(ii >= 0 && ii < (i64) len, "i OOB: {} ({}) < {}", ii, i, len);
        DEBUG_ASSERT(jj >= 0 && jj < (i64) len, "j OOB: {} ({}) < {}", jj, j, len);

        StringSlice c = *this;
        c.data = c.data + ii;
        c.len = jj - ii + 1;
        c.null_term = ii + c.len == len ? this->null_term : false;
        return c;
    }

    CXB_MAYBE_INLINE const char* c_str() const {
        return null_term ? data : nullptr;
    }

    CXB_MAYBE_INLINE int compare(const StringSlice& o) const {
        int result = memcmp(data, o.data, len < o.len ? len : o.len);
        if(result == 0) {
            return len - o.len;
        }
        return result;
    }

    CXB_MAYBE_INLINE bool operator==(const StringSlice& o) const {
        return compare(o) == 0;
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
            size_t len : 63;
            bool null_term : 1;
        };
        size_t metadata;
    };
    size_t capacity;
    Allocator* allocator;

    // ** SECTION: slice compatible methods - delegate to StringSlice
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
    CXB_MAYBE_INLINE operator StringSlice() const {
        return *reinterpret_cast<const StringSlice*>(this);
    }

    CXB_MAYBE_INLINE StringSlice slice(i64 i = 0, i64 j = -1) const {
        return reinterpret_cast<const StringSlice*>(this)->slice(i, j);
    }

    CXB_MAYBE_INLINE const char* c_str() const {
        return null_term ? data : nullptr;
    }

    CXB_MAYBE_INLINE bool operator==(const StringSlice& o) const {
        return reinterpret_cast<const StringSlice*>(this)->operator==(o);
    }

    CXB_MAYBE_INLINE bool operator!=(const StringSlice& o) const {
        return !(*this == o);
    }

    CXB_MAYBE_INLINE int compare(const StringSlice& o) const {
        return reinterpret_cast<const StringSlice*>(this)->compare(o);
    }

    CXB_MAYBE_INLINE bool operator<(const StringSlice& o) const {
        return reinterpret_cast<const StringSlice*>(this)->operator<(o);
    }

    CXB_MAYBE_INLINE bool operator>(const StringSlice& o) const {
        return o < *this;
    }

    // ** SECTION: allocator-related methods - delegate to UString
    CXB_MAYBE_INLINE MString& copy_(Allocator* to_allocator = &default_alloc) {
        MString temp = *this;
        *this = move(this->copy(to_allocator));
        temp.destroy();
        return *this;
    }

    CXB_MAYBE_INLINE MString copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        REQUIRES(to_allocator != nullptr);

        MString result{.data = nullptr, .len = len, .null_term = null_term, .capacity = 0, .allocator = to_allocator};
        if(len > 0) {
            result.reserve(len + null_term);
            memcpy(result.data, data, len);
            if(null_term) {
                result.data[len] = '\0';
            }
        }
        return result;
    }

    CXB_MAYBE_INLINE const char* c_str_maybe_copy(Allocator* copy_alloc_if_not) {
        if(!null_term) {
            ensure_null_terminated(copy_alloc_if_not);
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
        REQUIRES(allocator != nullptr);

        size_t old_count = capacity;
        size_t new_count = cap < CXB_STR_MIN_CAP ? CXB_STR_MIN_CAP : cap;
        if(new_count > old_count) {
            data = allocator->realloc(data, old_count, false, new_count);
            capacity = new_count;
        }
    }

    void resize(size_t new_len, char fill_char = '\0') {
        REQUIRES(UNLIKELY(allocator != nullptr));

        size_t reserve_size = new_len + null_term;
        if(capacity < reserve_size) {
            reserve(reserve_size);
        }

        size_t old_len = len;
        if(fill_char != '\0' && new_len > old_len) {
            memset(data + old_len, fill_char, new_len - old_len);
        }
        if(null_term) {
            data[new_len] = '\0';
        }
        len = new_len;
    }

    CXB_MAYBE_INLINE void push_back(char c) {
        if(n_bytes() + 1 >= capacity) {
            reserve(CXB_STR_GROW_FN(capacity));
        }
        data[len] = c;
        null_term |= (c == '\0');
        len += 1;
        if(null_term) {
            data[len] = '\0';
        }
    }

    CXB_MAYBE_INLINE char& push() {
        push_back('\0');
        return data[len - 1];
    }

    CXB_MAYBE_INLINE char pop_back() {
        REQUIRES(len > 0);
        char ret = data[len - 1];
        len--;
        data[len] = '\0';
        null_term = true;
        return ret;
    }

    void extend(StringSlice other) {
        if(other.len == 0) return;
        reserve(len + other.len);
        memcpy(data + len, other.data, other.len);
        len += other.len;
        if(null_term) {
            data[len] = '\0';
        }
    }

    CXB_MAYBE_INLINE void operator+=(StringSlice other) {
        this->extend(other);
    }

    CXB_MAYBE_INLINE void extend(const char* str, size_t n = SIZE_MAX) {
        if(!str) {
            return;
        }
        size_t n_len = n == SIZE_MAX ? strlen(str) : n;
        this->extend(StringSlice{.data = const_cast<char*>(str), .len = n_len, .null_term = true});
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
struct AString : MString {
    AString(Allocator* allocator = &default_alloc)
        : MString{.data = nullptr, .len = 0, .null_term = true, .capacity = 0, .allocator = allocator} {}

    AString(const MString& m)
        : MString{.data = m.data,
                  .len = m.len,
                  .null_term = m.null_term,
                  .capacity = m.capacity,
                  .allocator = m.allocator} {}

    AString(const char* cstr, size_t n = SIZE_MAX, bool null_term = true, Allocator* allocator = &default_alloc)
        : MString{.data = nullptr,
                  .len = n == SIZE_MAX ? strlen(cstr) : n,
                  .null_term = null_term,
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
        : MString{.data = nullptr, .len = 0, .null_term = true, .capacity = 0, .allocator = o.allocator} {
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

    CXB_MAYBE_INLINE AString& copy_(Allocator* to_allocator = &default_alloc) {
        *this = move(this->copy(to_allocator));
        return *this;
    }

    CXB_MAYBE_INLINE AString copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        REQUIRES(to_allocator != nullptr);

        AString result{data, len, null_term, to_allocator};
        return result;
    }

    CXB_MAYBE_INLINE MString release() {
        MString result{.data = data, .len = len, .null_term = null_term, .capacity = capacity, .allocator = allocator};
        this->allocator = nullptr;
        return result;
    }
};

template <typename T>
struct ArraySlice {
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
    CXB_MAYBE_INLINE ArraySlice<T> slice(i64 i = 0, i64 j = -1) {
        i64 ii = i < 0 ? len + i : i;
        i64 jj = j < 0 ? len + j : j;
        DEBUG_ASSERT(ii >= 0 && ii < (i64) len, "i OOB: {} ({}) < {}", ii, i, len);
        DEBUG_ASSERT(jj >= 0 && jj < (i64) len, "j OOB: {} ({}) < {}", jj, j, len);

        ArraySlice<T> c = *this;
        c.data = c.data + ii;
        c.len = jj - ii + 1;
        return c;
    }

    CXB_MAYBE_INLINE bool operator<(const ArraySlice<T>& o) const {
        size_t n = len < o.len ? len : o.len;
        for(size_t i = 0; i < n; ++i) {
            if(o.data[i] < data[i])
                return false;
            else if(data[i] < o.data[i])
                return true;
        }
        return len < o.len;
    }

    CXB_MAYBE_INLINE bool operator==(const ArraySlice<T>& o) const {
        if(len != o.len) return false;
        for(size_t i = 0; i < len; ++i) {
            if(!(data[i] == o.data[i])) return false;
        }
        return true;
    }

    CXB_MAYBE_INLINE bool operator!=(const ArraySlice<T>& o) const {
        return !(*this == o);
    }

    CXB_MAYBE_INLINE bool operator>(const ArraySlice<T>& o) const {
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
};

template <typename T>
struct MArray {
    T* data;
    size_t len;
    size_t capacity;
    Allocator* allocator;

    MArray(Allocator* allocator = &default_alloc) : data{nullptr}, len{0}, capacity{0}, allocator{allocator} {}
    MArray(T* data, size_t len, Allocator* allocator = &default_alloc)
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
    CXB_MAYBE_INLINE ArraySlice<T> slice(i64 i = 0, i64 j = -1) {
        i64 ii = i < 0 ? len + i : i;
        i64 jj = j < 0 ? len + j : j;
        DEBUG_ASSERT(ii >= 0 && ii < (i64) len, "i OOB: {} ({}) < {}", ii, i, len);
        DEBUG_ASSERT(jj >= 0 && jj < (i64) len, "j OOB: {} ({}) < {}", jj, j, len);

        ArraySlice<T> c = *this;
        c.data = c.data + ii;
        c.len = jj - ii + 1;
        return c;
    }

    CXB_MAYBE_INLINE bool operator<(const ArraySlice<T>& o) const {
        size_t n = len < o.len ? len : o.len;
        for(size_t i = 0; i < n; ++i) {
            if(o.data[i] < data[i])
                return false;
            else if(data[i] < o.data[i])
                return true;
        }
        return len < o.len;
    }

    CXB_MAYBE_INLINE bool operator==(const ArraySlice<T>& o) const {
        if(len != o.len) return false;
        for(size_t i = 0; i < len; ++i) {
            if(!(data[i] == o.data[i])) return false;
        }
        return true;
    }

    CXB_MAYBE_INLINE bool operator!=(const ArraySlice<T>& o) const {
        return !(*this == o);
    }

    CXB_MAYBE_INLINE bool operator>(const ArraySlice<T>& o) const {
        return o < *this;
    }

    CXB_MAYBE_INLINE operator ArraySlice<T>() const {
        return *reinterpret_cast<const ArraySlice<T>*>(this);
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
    CXB_MAYBE_INLINE MArray<T>& copy_(Allocator* to_allocator = &default_alloc) {
        *this = move(this->copy(to_allocator));
        return *this;
    }

    CXB_MAYBE_INLINE MArray<T> copy(Allocator* to_allocator = nullptr) {
        if(to_allocator == nullptr) to_allocator = allocator;
        REQUIRES(to_allocator != nullptr);

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
        REQUIRES(allocator != nullptr);

        size_t old_count = capacity;
        size_t new_count = cap < CXB_STR_MIN_CAP ? CXB_STR_MIN_CAP : cap;
        if(new_count > old_count) {
            data = allocator->realloc(data, old_count, std::is_trivially_default_constructible_v<T>, new_count);
            capacity = new_count;
        }
    }

    CXB_MAYBE_INLINE void resize(size_t new_len) {
        REQUIRES(UNLIKELY(allocator != nullptr));

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
        REQUIRES(UNLIKELY(allocator != nullptr));

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
        REQUIRES(UNLIKELY(allocator != nullptr));
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

    template <class O>
    void release(O& out) {
        out.data = data;
        out.len = len;
        out.capacity = capacity;
        out.allocator = allocator;
        allocator = nullptr;
    }

    void extend(ArraySlice<T> other) {
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

template <class T, class O>
static constexpr MArray<T> marray_from_pod(O o, Allocator* allocator) {
    return MArray<T>{o.data, o.len, o.capacity, allocator};
}

template <class T, class O>
static constexpr MArray<T> marray_from_pod(O* o, Allocator* allocator) {
    return MArray<T>{o->data, o->len, o->capacity, allocator};
}

template <typename T>
struct AArray : MArray<T> {
    AArray(Allocator* allocator = &default_alloc) : MArray<T>(allocator) {}
    AArray(T* data, size_t len, Allocator* allocator = &default_alloc) : MArray<T>(data, len, allocator) {}
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

/* SECTION: variant types */

template <typename T, typename EC>
struct Result {
    T value;
    EC error;

    // TODO: place on "error" Arena*
    StringSlice reason;

    inline operator bool() const {
        return (i64)error == 0;
    }
};

template <typename T>
struct Optional {
    T value;
    bool exists;
};
CXB_NS_END

/* SECTION: C-compat API (continued) */
// TODO: move to cxb-c.h ?
struct Arena;
CXB_C_EXPORT void* arena_push(Arena* arena, size_t size, size_t align);
CXB_C_EXPORT void arena_pop_to(Arena* arena, u64 pos);
CXB_C_EXPORT void arena_clear(Arena* arena);

struct ArenaParams {
    size_t reserve_bytes;
    size_t max_n_blocks;
};

struct Arena {
    char* start;
    char* end;
    size_t pos;
    Arena* next;
    Arena* prev;
    size_t n_blocks;
    ArenaParams params;
};

CXB_C_EXPORT Arena* arena_make(ArenaParams params);
CXB_C_EXPORT Arena* arena_make_nbytes(size_t n_bytes);
CXB_C_EXPORT void arena_destroy(Arena* arena);

struct ArenaTemp {
    Arena* arena;
    u64 pos;
};

template <typename T>
inline T* push(Arena* arena, size_t n = 1) {
    const size_t size = sizeof(T) * n;
    T* data = (T*) arena_push(arena, size, 0);
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
inline void pop(Arena* arena, T* x) {
    ASSERT((void*) (x) >= (void*) arena->start && (void*) (x) < arena->end, "array not allocated on arena");
    ASSERT((void*) (x + 1) == (void*) (arena->start + arena->pos), "cannot pop unless array is at the end");
    arena->pos -= sizeof(T);
}

// *SECTION: StringSlice arena functions
inline StringSlice push_str(Arena* arena, size_t n = 1) {
    REQUIRES(n > 0);
    char* data = push<char>(arena, n);
    return StringSlice{.data = data, .len = n - 1, .null_term = true};
}

inline StringSlice push_str(Arena* arena, StringSlice to_copy) {
    char* data = push<char>(arena, to_copy.n_bytes());
    StringSlice result = StringSlice{.data = data, .len = to_copy.len, .null_term = to_copy.null_term};
    memccpy(result.data, to_copy.data, sizeof(char), to_copy.n_bytes());
    return result;
}

inline void resize(Arena* arena, StringSlice& str, size_t n, char fill_char = '\0') {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");
    ASSERT(n > str.size());

    size_t delta = n - str.size();
    arena_push(arena, delta, 0);
    memset(str.data + str.len, fill_char, delta);
    str.len += delta;
}

inline void push_back(Arena* arena, StringSlice& str, char ch) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    str.data[str.len] = ch;
    str.len += 1;
    if(str.null_term) {
        str.data[str.len] = '\0';
    }
    arena->pos += sizeof(char);
}

inline void pop_back(Arena* arena, StringSlice& str) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    str.len -= 1;
    str.data[str.len] = '\0';
    str.null_term = true;
    arena_pop_to(arena, arena->pos - 1);
}

inline void pop_all(Arena* arena, StringSlice& str) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    arena_pop_to(arena, arena->pos - str.n_bytes());
    str.len = 0;
    str.data = nullptr;
}

inline void insert(Arena* arena, StringSlice& str, char ch, size_t i) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");
    ASSERT(i <= str.len, "insert position out of bounds");

    push_back(arena, str, '\0');
    memmove(str.data + i + 1, str.data + i, str.len - i - 1);
    str.data[i] = ch;
}

inline void insert(Arena* arena, StringSlice& str, StringSlice to_insert, size_t i) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");
    ASSERT(i <= str.len, "insert position out of bounds");

    arena_push(arena, to_insert.len, 0);

    size_t old_len = str.len;
    str.len += to_insert.len;

    memmove(str.data + i + to_insert.len, str.data + i, old_len - i);
    memcpy(str.data + i, to_insert.data, to_insert.len);
}

inline void extend(Arena* arena, StringSlice& str, StringSlice to_append) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    arena_push(arena, to_append.len, 0);

    size_t old_len = str.len;
    str.len += to_append.len;
    memcpy(str.data + old_len, to_append.data, to_append.len);
}

// *SECTION: ArraySlice arena functions
template <typename T>
inline ArraySlice<T> push_array(Arena* arena, size_t n) {
    T* data = push<T>(arena, n);
    return ArraySlice<T>{.data = data, .len = n};
}

template <typename T>
inline ArraySlice<T> push_array(Arena* arena, ArraySlice<T> to_copy) {
    T* data = push<T>(arena, to_copy.len);
    if constexpr(std::is_trivially_copyable_v<T>) {
        memcpy(data, to_copy.data, to_copy.len * sizeof(T));
    } else {
        for(size_t i = 0; i < to_copy.len; ++i) {
            data[i] = to_copy.data[i];
        }
    }
    return ArraySlice<T>{.data = data, .len = to_copy.len};
}

template <class T>
inline void push_back(Arena* arena, ArraySlice<T>& xs, T value) {
    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot push unless array is at the end");
    arena_push(arena, sizeof(T), 0);
    xs.data[xs.len] = value;
    xs.len += 1;
}

template <class T, class... Args>
inline void emplace_back(Arena* arena, ArraySlice<T>& xs, Args&&... args) {
    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot push unless array is at the end");
    arena_push(arena, sizeof(T), 0);
    xs.data[xs.len] = T{forward<Args>(args)...};
    xs.len += 1;
}

template <typename T>
inline void pop_back(Arena* arena, ArraySlice<T>& xs) {
    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot pop unless array is at the end");
    arena_pop_to(arena, arena->pos - sizeof(xs.data));

    if constexpr(!std::is_trivially_destructible_v<T>) {
        xs.data[xs.len].~T();
    }
    xs.len -= 1;
}

template <typename T>
inline void insert(Arena* arena, ArraySlice<T>& xs, ArraySlice<T> to_insert, size_t i) {
    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot push unless array is at the end");
    ASSERT(i <= xs.len, "insert position out of bounds");

    arena_push(arena, to_insert.len * sizeof(T), 0);

    size_t old_len = xs.len;
    xs.len += to_insert.len;

    memmove(xs.data + i + to_insert.len, xs.data + i, (old_len - i) * sizeof(T));
    memcpy(xs.data + i, to_insert.data, to_insert.len * sizeof(T));
}

template <typename T>
inline void extend(Arena* arena, ArraySlice<T>& xs, ArraySlice<T> to_append) {
    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot push unless array is at the end");

    arena_push(arena, to_append.len * sizeof(T), 0);

    size_t old_len = xs.len;
    xs.len += to_append.len;

    memcpy(xs.data + old_len, to_append.data, to_append.len * sizeof(T));
}

template <typename T>
inline void pop_all(Arena* arena, ArraySlice<T>& xs) {
    ASSERT((void*) xs.data >= (void*) arena->start && (void*) xs.data < arena->end, "array not allocated on arena");
    ASSERT((void*) (xs.data + xs.len) == (void*) (arena->start + arena->pos), "cannot pop unless array is at the end");
    arena_pop_to(arena, arena->pos - (sizeof(xs.data) * xs.len));
    xs.data = nullptr;
    xs.len = 0;
}

// TODO
// struct ArenaAlloc: Allocator {
//     Arena();
// };

#define S8_STR(s) (StringSlice{.data = (char*) s.c_str(), .len = (size_t) s.size(), .null_term = true})

#ifdef CXB_IMPL
#include "cxb.cpp"
#endif

// ensure the C API is included
#define CXB_C_API
#include "cxb-c.h"

#endif
