#include "cxb.h"

#include <stdlib.h> // for malloc, free, realloc, calloc

#ifdef __APPLE__
#include <sys/mman.h>
#include <unistd.h> // for sysconf()
#endif

// TODO: remove fmtlib
#include "fmt/format.h"
#include "format.cc"

/*
NOTES on Arenas

# mmap

To commit memory read or write to the memory, e.g. memset(base, 0, commit_n_bytes);

To de-commit memory, call mprotect:

    size_t decommit = 5 * 1024 * 1024;
    char* decommit_addr = base - decommit;
    if (madvise(decommit_addr, decommit, MADV_FREE) != 0) {
        perror("madvise free failed");
    }
    mprotect(decommit_addr, decommit, PROT_NONE);
*/

CXB_C_EXPORT Arena* arena_make(ArenaParams params) {
    if(params.reserve_bytes == 0) {
        params.reserve_bytes = MB(1);
    }
    ASSERT(params.reserve_bytes > 2 * sizeof(Arena), "need memory to allocate arena");

    // TODO: can reserve specific regions with PROT_NONE
    void* data = (char*) mmap(nullptr, params.reserve_bytes, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if(data == MAP_FAILED) {
        return nullptr;
    }

    Arena* result = (Arena*) data;
    ASAN_UNPOISON_MEMORY_REGION(result, sizeof(Arena));
    memset(result, 0, sizeof(Arena));

    result->params = params;
    result->start = (char*) data;
    result->pos = sizeof(Arena);
    result->end = result->start + params.reserve_bytes;
    result->n_blocks = 1;
    return result;
}

CXB_C_EXPORT Arena* arena_make_nbytes(size_t n_bytes) {
    return arena_make(ArenaParams{.reserve_bytes = n_bytes, .max_n_blocks = 1});
}

CXB_C_EXPORT void* arena_push_bytes(Arena* arena, size_t size, size_t align) {
    ASSERT(UNLIKELY(arena != nullptr), "expected an arena");
    u64 padding = (-arena->pos) & (align - 1);
    arena->pos += padding;
    ASSERT(arena->start + arena->pos + size < arena->end, "arena will spill");

    void* data = arena->start + arena->pos;
    // data % align == 0, but align = 2^x
    DEBUG_ASSERT(((u64) data & (align - 1)) == 0);
    ASAN_UNPOISON_MEMORY_REGION(data, size);
    arena->pos += size;
    return data;
}

CXB_C_EXPORT void arena_pop_to(Arena* arena, u64 pos) {
    if(UNLIKELY(pos == arena->pos)) {
        return;
    }

    ASSERT(pos >= 0 && pos < arena->pos && pos <= (u64) (arena->end - arena->start), "pop_to pos out of bounds");
    u64 old_pos = arena->pos;
    arena->pos = pos;
    ASAN_POISON_MEMORY_REGION(arena->start + old_pos, old_pos - pos);
}

CXB_C_EXPORT void arena_clear(Arena* arena) {
    arena->pos = 0;
}

CXB_C_EXPORT void arena_destroy(Arena* arena) {
    ASSERT(arena != nullptr);
    ASSERT(arena->start != nullptr);
    munmap(arena->start, arena->end - arena->start);
}

// * SECTION: allocators
void* heap_alloc_proc(void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes, bool fill_zeros, void* data);
void heap_free_proc(void* head, size_t n_bytes, void* data);
void heap_free_all_proc(void* data);

HeapAllocData heap_alloc_data = {};

Allocator heap_alloc = {.alloc_proc = heap_alloc_proc,
                        .free_proc = heap_free_proc,
                        .free_all_proc = heap_free_all_proc,
                        .data = (void*) &heap_alloc_data};

void* heap_alloc_proc(void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes, bool fill_zeros, void* data) {
    (void) alignment;

    HeapAllocData* heap_data = (HeapAllocData*) data;
    heap_data->n_active_bytes += (n_bytes - old_n_bytes);
    heap_data->n_allocated_bytes += (n_bytes - old_n_bytes);

    if(old_n_bytes > 0) {
        ASSERT(head != nullptr);
        ASSERT(n_bytes > old_n_bytes);

        void* data = realloc(head, n_bytes);
        if(fill_zeros) {
            if(!data) {
                data = calloc(n_bytes, 1);
                memcpy(data, head, old_n_bytes);
            } else {
                memset((char*) (data) + old_n_bytes, 0, n_bytes - old_n_bytes);
            }

        } else if(!data) {
            data = malloc(n_bytes);
            memcpy(data, head, old_n_bytes);
            heap_data->n_freed_bytes += old_n_bytes;
            free(head);
        }

        return data;
    } else {
        ASSERT(head == nullptr);

        if(fill_zeros) {
            return calloc(n_bytes, 1);
        }
        return malloc(n_bytes);
    }
}

void heap_free_proc(void* head, size_t n_bytes, void* data) {
    HeapAllocData* heap_data = (HeapAllocData*) data;
    free(head);
    heap_data->n_active_bytes -= n_bytes;
    heap_data->n_freed_bytes += n_bytes;
}

void heap_free_all_proc(void* data) {
    (void) data;
    INVALID_CODEPATH("heap allocator does not support free all");
}

String8 arena_push_string8(Arena* arena, size_t n) {
    ASSERT(n > 0);
    char* data = arena_push<char>(arena, n);
    return String8{.data = data, .len = n - 1, .not_null_term = false};
}

String8 arena_push_string8(Arena* arena, String8 to_copy) {
    char* data = arena_push<char>(arena, to_copy.n_bytes());
    String8 result = String8{.data = data, .len = to_copy.len, .not_null_term = to_copy.not_null_term};
    memccpy(result.data, to_copy.data, sizeof(char), to_copy.n_bytes());
    return result;
}

void string8_resize(String8& str, Arena* arena, size_t n, char fill_char) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    if(n > str.len) {
        size_t delta = n - str.len;
        arena_push<char>(arena, delta);
        memset(str.data + str.len, fill_char, delta);
        str.len += delta;
    } else {
        size_t pos = arena->pos - (sizeof(char) * (str.len - n));
        arena_pop_to(arena, pos);
        str.len = n;
    }
}

void string8_push_back(String8& str, Arena* arena, char ch) {
    ASSERT(str.data == nullptr || (void*) str.data >= (void*) arena->start && (void*) str.data < arena->end,
           "string not allocated on arena");
    ASSERT(str.data == nullptr || (void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    char* data = arena_push<char>(arena, str.data == nullptr && str.len == 0 && !str.not_null_term ? 2 : 1);
    str.data = UNLIKELY(str.data == nullptr) ? data : str.data;
    str.data[str.len] = ch;
    str.len += 1;
    str.not_null_term = !(!str.not_null_term || ch == '\0');
}

void string8_pop_back(String8& str, Arena* arena) {
    ASSERT(str.len > 0, "empty string provided to string8_pop_back");
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    arena_pop_to(arena, arena->pos - sizeof(char));
    str.len -= 1;
    str.data[str.len] = '\0';
    str.not_null_term = false;
}

void string8_pop_all(String8& str, Arena* arena) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    arena_pop_to(arena, arena->pos - str.n_bytes());
    str.len = 0;
    str.data = nullptr;
}

void string8_insert(String8& str, Arena* arena, char ch, size_t i) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");
    ASSERT(i <= str.len, "insert position out of bounds");

    arena_push<char>(arena, 1);
    str.len += 1;
    memmove(str.data + i + 1, str.data + i, str.len - i - 1);
    str.data[i] = ch;
}

void string8_insert(String8& str, Arena* arena, String8 to_insert, size_t i) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");
    ASSERT(i <= str.len, "insert position out of bounds");

    arena_push<char>(arena, to_insert.len);

    size_t old_len = str.len;
    str.len += to_insert.len;

    memcpy(str.data + i + to_insert.len, str.data + i, old_len - i);
    memcpy(str.data + i, to_insert.data, to_insert.len);
}

void string8_extend(String8& str, Arena* arena, String8 to_append) {
    ASSERT(str.data == nullptr || (void*) str.data >= (void*) arena->start && (void*) str.data < arena->end,
           "string not allocated on arena");
    ASSERT(str.data == nullptr || (void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    void* data = arena_push<char>(arena, to_append.len);
    str.data = UNLIKELY(str.data == nullptr) ? (char*) data : str.data;

    size_t old_len = str.len;
    str.len += to_append.len;
    memcpy(str.data + old_len, to_append.data, to_append.len);
}

thread_local ThreadLocalRuntime cxb_runtime = {};
static CxbRuntimeParams runtime_params = {};

CXB_INLINE void _maybe_init_runtime() {
    if(UNLIKELY(!cxb_runtime.perm)) {
        cxb_runtime.perm = arena_make(runtime_params.perm_params);
        cxb_runtime.scratch[0] = arena_make(runtime_params.scratch_params);
        cxb_runtime.scratch[1] = arena_make(runtime_params.scratch_params);
        cxb_runtime.scratch_idx = 0;
    }
}

void cxb_init(CxbRuntimeParams params) {
    runtime_params = params;
}

Arena* get_perm() {
    _maybe_init_runtime();
    return cxb_runtime.perm;
}

ArenaTmp begin_scratch() {
    _maybe_init_runtime();
    Arena* result = cxb_runtime.scratch[cxb_runtime.scratch_idx];
    cxb_runtime.scratch_idx += 1;
    cxb_runtime.scratch_idx %= 2;
    return ArenaTmp{result, result->pos};
}

void end_scratch(const ArenaTmp& tmp) {
    arena_pop_to(tmp.arena, tmp.pos);
}

void format_value(Arena* a, String8& dst, String8 args, const char* s) {
    (void) args;
    while(*s) {
        string8_push_back(dst, a, *s++);
    }
}

void format_value(Arena* a, String8& dst, String8 args, String8 s) {
    (void) args;
    string8_extend(dst, a, s);
}

// TODO: remove fmtlib
struct String8AppendIt {
    Arena* a;
    String8* dst;

    using difference_type = std::ptrdiff_t;

    String8AppendIt& operator=(char c) {
        string8_push_back(*dst, a, c);
        return *this;
    }
    String8AppendIt& operator*() {
        return *this;
    }
    String8AppendIt& operator++() {
        return *this;
    }
    String8AppendIt operator++(int) {
        return *this;
    }
};

template <class T>
std::enable_if_t<std::is_floating_point_v<T>, void> format_float_impl(Arena* a, String8& dst, String8 args, T value) {
    i64 int_part = static_cast<i64>(value);
    f64 frac = value - int_part;
    if(frac < 0) frac *= -1;

    ParseResult<u64> digits = args.slice(1, args.len && args.back() == 'f' ? -2 : -1).parse<u64>();
    u64 n_digits = digits ? min((u64) std::numeric_limits<T>::max_digits10, digits.value) : 3;

    String8AppendIt out{a, &dst};
    // TODO: remove fmtlib
    // TODO: implement Dragonbox
    if(digits.exists) {
        fmt::format_to(out, "{:.{}f}", value, static_cast<int>(n_digits));
    } else {
        fmt::format_to(out, "{:.{}g}", value, static_cast<int>(n_digits));
    }
}

void format_value(Arena* a, String8& dst, String8 args, bool value) {
    (void) args;
    dst.extend(a, value ? S8_LIT("true") : S8_LIT("false"));
}

void format_value(Arena* a, String8& dst, String8 args, f32 value) {
    format_float_impl(a, dst, args, value);
}

void format_value(Arena* a, String8& dst, String8 args, f64 value) {
    format_float_impl(a, dst, args, value);
}

CXB_C_EXPORT bool utf8_iter_next(Utf8Iter* iter, Utf8IterBatch* batch) {
    // TODO: use simd
    batch->len = 0;
    while(iter->pos < iter->s.len) {
        u8 c1 = iter->s[iter->pos];
        // 1 byte
        if(!(c1 & (1 << 7))) {
            batch->data[batch->len++] = c1;
            iter->pos += 1;
        }
        // 1, 2, or 3 bytes ahead
        else {
            // +3 bytes
            if((c1 & 0b11111000) == 0b11110000) {
                u8 c2 = iter->s[iter->pos + 1];
                u8 c3 = iter->s[iter->pos + 2];
                u8 c4 = iter->s[iter->pos + 3];
                batch->data[batch->len++] = (((c1 & 0b00000111) << 18) + ((c2 & 0b00111111) << 12) +
                                             ((c3 & 0b00111111) << 6) + ((c4 & 0b00111111) << 0));

                iter->pos += 4;
            }
            // +2 bytes
            else if((c1 & 0b11110000) == 0b11100000) {
                u8 c2 = iter->s[iter->pos + 1];
                u8 c3 = iter->s[iter->pos + 2];
                batch->data[batch->len++] =
                    (((c1 & 0b00001111) << 12) + ((c2 & 0b00111111) << 6) + ((c3 & 0b00111111) << 0));
                iter->pos += 3;
            }
            // +1 bytes
            else if((c1 & 0b11100000) == 0b11000000) {
                u8 c2 = iter->s[iter->pos + 1];
                batch->data[batch->len++] = (((c1 & 0b00011111) << 6) + ((c2 & 0b00111111) << 0));
                iter->pos += 2;
            } else {
                // TODO: error ?
                iter->pos += 1;
            }
        }
    }
    return batch->len > 0;
}
