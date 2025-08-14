#include "cxb.h"

#include <stdlib.h> // for malloc, free, realloc, calloc

#ifdef __APPLE__
#include <sys/mman.h>
#include <unistd.h> // for sysconf()
#endif

/*
NOTES on Arenas

# mmap

To commit memory read or write to the memory, e.g. memset(base, 0, commit_n_bytes);

To decommite memory, call mprotect:

    size_t decommit = 5 * 1024 * 1024;
    char* decommit_addr = base - decommit;
    if (madvise(decommit_addr, decommit, MADV_FREE) != 0) {
        perror("madvise free failed");
    }
    mprotect(decommit_addr, decommit, PROT_NONE);
*/

CXB_C_EXPORT Arena* arena_make(ArenaParams params) {
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
    // ASAN_UNPOISON_MEMORY_REGION(arena->start + arena->pos, padding);
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

CXB_C_EXPORT void cxb_mstring_destroy(MString* s) {
    s->destroy();
}

CXB_C_EXPORT void cxb_mstring_reserve(MString* s, size_t cap) {
    if(!s) return;
    s->reserve(cap);
}

CXB_C_EXPORT void cxb_mstring_resize(MString* s, size_t size) {
    if(!s) return;
    s->resize(size);
}

CXB_C_EXPORT void cxb_mstring_extend(MString* s, String8 slice) {
    if(!s) return;
    s->extend(slice);
}

CXB_C_EXPORT void cxb_mstring_push_back(MString* s, char val) {
    if(!s) return;
    s->push_back(val);
}

CXB_C_EXPORT char* cxb_mstring_push(MString* s) {
    if(!s) return nullptr;
    return &s->push();
}

CXB_C_EXPORT void cxb_mstring_ensure_null_terminated(MString* s) {
    if(!s) return;
    s->ensure_null_terminated();
}

CXB_C_EXPORT MString cxb_mstring_copy(MString s, Allocator* to_allocator) {
    return s.copy(to_allocator);
}

// ** SECTION: String8 C functions
CXB_C_EXPORT size_t cxb_ss_size(String8 s) {
    return s.size();
}

CXB_C_EXPORT size_t cxb_ss_n_bytes(String8 s) {
    return s.n_bytes();
}

CXB_C_EXPORT bool cxb_ss_empty(String8 s) {
    return s.empty();
}

CXB_C_EXPORT const char* cxb_ss_c_str(String8 s) {
    return s.c_str();
}

CXB_C_EXPORT String8 cxb_ss_slice(String8 s, i64 i, i64 j) {
    return s.slice(i, j);
}

CXB_C_EXPORT bool cxb_ss_eq(String8 a, String8 b) {
    return a == b;
}

CXB_C_EXPORT bool cxb_ss_neq(String8 a, String8 b) {
    return a != b;
}

CXB_C_EXPORT bool cxb_ss_lt(String8 a, String8 b) {
    return a < b;
}

CXB_C_EXPORT char cxb_ss_back(String8 s) {
    return s.back();
}

// ** SECTION: MString C functions (inline ones)
CXB_C_EXPORT size_t cxb_mstring_size(MString s) {
    return s.size();
}

CXB_C_EXPORT size_t cxb_mstring_n_bytes(MString s) {
    return s.n_bytes();
}

CXB_C_EXPORT bool cxb_mstring_empty(MString s) {
    return s.empty();
}

CXB_C_EXPORT const char* cxb_mstring_c_str(MString s) {
    return s.c_str();
}

CXB_C_EXPORT bool cxb_mstring_eq(MString a, MString b) {
    return a == b;
}

CXB_C_EXPORT bool cxb_mstring_neq(MString a, MString b) {
    return a != b;
}

CXB_C_EXPORT bool cxb_mstring_lt(MString a, MString b) {
    return a < b;
}

CXB_C_EXPORT char cxb_mstring_back(MString s) {
    return s.back();
}

CXB_C_EXPORT size_t cxb_mstring_capacity(MString s) {
    return s.capacity;
}

String8 arena_push_string8(Arena* arena, size_t n) {
    ASSERT(n > 0);
    char* data = arena_push<char>(arena, n);
    return String8{.data = data, .len = n - 1, .null_term = true};
}

String8 arena_push_string8(Arena* arena, String8 to_copy) {
    char* data = arena_push<char>(arena, to_copy.n_bytes());
    String8 result = String8{.data = data, .len = to_copy.len, .null_term = to_copy.null_term};
    memccpy(result.data, to_copy.data, sizeof(char), to_copy.n_bytes());
    return result;
}

void string8_resize(String8& str, Arena* arena, size_t n, char fill_char) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");
    ASSERT(n > str.size());

    size_t delta = n - str.size();
    arena_push(arena, delta, alignof(char));
    memset(str.data + str.len, fill_char, delta);
    str.len += delta;
}

void string8_push_back(String8& str, Arena* arena, char ch) {
    ASSERT(str.data == nullptr || (void*) str.data >= (void*) arena->start && (void*) str.data < arena->end,
           "string not allocated on arena");
    ASSERT(str.data == nullptr || (void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    void* data = arena_push(arena, sizeof(char), alignof(char));
    str.data = UNLIKELY(str.data == nullptr) ? (char*) data : str.data;
    str.data[str.len] = ch;
    str.len += 1;
    if(str.null_term) {
        str.data[str.len] = '\0';
    }
}

void string8_pop_back(String8& str, Arena* arena) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    arena_pop_to(arena, arena->pos - 1);
    str.len -= 1;
    str.data[str.len] = '\0';
    str.null_term = true;
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

    string8_push_back(str, arena, '\0');
    memmove(str.data + i + 1, str.data + i, str.len - i - 1);
    str.data[i] = ch;
}

void string8_insert(String8& str, Arena* arena, String8 to_insert, size_t i) {
    ASSERT((void*) str.data >= (void*) arena->start && (void*) str.data < arena->end, "string not allocated on arena");
    ASSERT((void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");
    ASSERT(i <= str.len, "insert position out of bounds");

    arena_push(arena, to_insert.len, alignof(char));

    size_t old_len = str.len;
    str.len += to_insert.len;

    memmove(str.data + i + to_insert.len, str.data + i, old_len - i);
    memcpy(str.data + i, to_insert.data, to_insert.len);
}

void string8_extend(String8& str, Arena* arena, String8 to_append) {
    ASSERT(str.data == nullptr || (void*) str.data >= (void*) arena->start && (void*) str.data < arena->end,
           "string not allocated on arena");
    ASSERT(str.data == nullptr || (void*) (str.data + str.n_bytes()) == (void*) (arena->start + arena->pos),
           "cannot push unless array is at the end");

    void* data = arena_push(arena, to_append.len, alignof(char));
    str.data = UNLIKELY(str.data == nullptr) ? (char*) data : str.data;

    size_t old_len = str.len;
    str.len += to_append.len;
    memcpy(str.data + old_len, to_append.data, to_append.len);
}
