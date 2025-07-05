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
    return max(size_t{CXB_MALLOCATOR_MIN_CAP}, CXB_MALLOCATOR_GROW_FN(count));
}

void* mallocator_alloc_impl(
    Allocator* a, bool fill_zeros, void* head, size_t n_bytes, size_t alignment, size_t old_n_bytes) {
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

        } else {
            if(!data) {
                data = malloc(n_bytes);
                memcpy(data, head, old_n_bytes);
            }
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
    if(s->data && s->allocator) {
        size_t* header = ((size_t*) s->data) - 1;
        size_t cap = *header;
        s->allocator->free_impl(s->allocator, (void*) header, cap + sizeof(size_t));
    }
    s->allocator = nullptr;
    s->data = nullptr;
    s->len = 0;
    s->null_term = false;
}
