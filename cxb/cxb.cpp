#include "cxb.h"

#include <stdlib.h> // for malloc, free, realloc, calloc

CXB_NS_BEGIN

Mallocator default_alloc = {};

// * SECTION: allocators
size_t mallocator_growth_sug_impl(const Allocator* a, size_t count);
void* mallocator_alloc_impl(
    Allocator* a, bool fill_zeros, void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes);
void malloctor_free_impl(Allocator* a, void* head, size_t n_bytes);

Mallocator::Mallocator() : Allocator{mallocator_growth_sug_impl, mallocator_alloc_impl, malloctor_free_impl} {}

size_t mallocator_growth_sug_impl(const Allocator* a, size_t count) {
    (void) a;
    return max(size_t{CXB_MALLOCATOR_MIN_CAP}, CXB_MALLOCATOR_GROW_FN(count));
}

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

CXB_C_EXPORT void cxb_mstring_ensure_capacity(MString* s, size_t cap) {
    if(!s) return;
    s->ensure_capacity(cap);
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

CXB_C_EXPORT void cxb_mstring_reserve(MString* s, size_t cap) {
    if(!s) return;
    s->reserve(cap);
}

CXB_C_EXPORT void cxb_mstring_ensure_null_terminated(MString* s) {
    if(!s) return;
    s->ensure_null_terminated();
}

CXB_C_EXPORT MString cxb_mstring_copy(MString s, Allocator* to_allocator) {
    return s.copy(to_allocator);
}
