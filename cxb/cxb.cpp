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

CXB_C_EXPORT Arena arena_make(ArenaParams params) {
    Arena result = {};
    result.params = params;

    result.start = (char*) mmap(nullptr, params.reserve_bytes, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if(result.start == MAP_FAILED) {
        result.start = nullptr;
        return result;
    }
    result.pos = 0;
    result.end = result.start + params.reserve_bytes;
    result.n_blocks = 1;
    return result;
}

CXB_C_EXPORT Arena arena_make_nbytes(size_t n_bytes) {
    return arena_make(ArenaParams{.reserve_bytes = n_bytes, .max_n_blocks = 1});
}

CXB_C_EXPORT void* arena_push(Arena* arena, size_t size, size_t align) {
    ASSERT(align == 0, "TODO support align > 0");
    void* data = arena->start + arena->pos;
    arena->pos += size;
    return data;
}

CXB_C_EXPORT void arena_pop_to(Arena* arena, u64 pos) {
    // ASSERT(pos >= 0 && pos < arena->pos && pos <= (arena->end - arena->start), "pop_to pos out of bounds");
    arena->pos = pos;
}

CXB_C_EXPORT void arena_clear(Arena* arena) {
    arena->pos = 0;
}

Mallocator default_alloc = {};

CXB_NS_BEGIN

// * SECTION: allocators
void* mallocator_alloc_impl(
    Allocator* a, bool fill_zeros, void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes);
void malloctor_free_impl(Allocator* a, void* head, size_t n_bytes);

Mallocator::Mallocator() : Allocator{mallocator_alloc_impl, malloctor_free_impl} {}

void* mallocator_alloc_impl(
    Allocator* a, bool fill_zeros, void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes) {
    (void) alignment;

    Mallocator* ma = static_cast<Mallocator*>(a);
    ma->n_active_bytes += (n_bytes - old_n_bytes);
    ma->n_allocated_bytes += (n_bytes - old_n_bytes);

    // TODO: alignment
    if(old_n_bytes > 0) {
        REQUIRES(head != nullptr);
        REQUIRES(n_bytes > old_n_bytes);

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
            ma->n_freed_bytes += old_n_bytes;
            free(head);
        }

        return data;
    } else {
        REQUIRES(head == nullptr);

        if(fill_zeros) {
            return calloc(n_bytes, 1);
        }
        return malloc(n_bytes);
    }
}

void malloctor_free_impl(Allocator* a, void* head, size_t n_bytes) {
    Mallocator* ma = static_cast<Mallocator*>(a);
    ma->n_active_bytes += -static_cast<i64>(n_bytes);
    ma->n_freed_bytes += n_bytes;
    free(head);
}

// TODO
// Arena::Arena() {}

CXB_NS_END

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

CXB_C_EXPORT void cxb_mstring_extend(MString* s, StringSlice slice) {
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

// ** SECTION: StringSlice C functions
CXB_C_EXPORT size_t cxb_ss_size(StringSlice s) {
    return s.size();
}

CXB_C_EXPORT size_t cxb_ss_n_bytes(StringSlice s) {
    return s.n_bytes();
}

CXB_C_EXPORT bool cxb_ss_empty(StringSlice s) {
    return s.empty();
}

CXB_C_EXPORT const char* cxb_ss_c_str(StringSlice s) {
    return s.c_str();
}

CXB_C_EXPORT StringSlice cxb_ss_slice(StringSlice s, i64 i, i64 j) {
    return s.slice(i, j);
}

CXB_C_EXPORT bool cxb_ss_eq(StringSlice a, StringSlice b) {
    return a == b;
}

CXB_C_EXPORT bool cxb_ss_neq(StringSlice a, StringSlice b) {
    return a != b;
}

CXB_C_EXPORT bool cxb_ss_lt(StringSlice a, StringSlice b) {
    return a < b;
}

CXB_C_EXPORT char cxb_ss_back(StringSlice s) {
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
