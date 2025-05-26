#include "cxb.h"

CXB_NS_BEGIN

thread_local Mallocator default_alloc = {};

// * SECTION: allocators
size_t mallocator_growth_sug_impl(const Allocator* a, size_t count);
void* mallocator_alloc_impl(Allocator* a, bool fill_zeros, void* head, size_t num_bytes, size_t alignment, size_t old_n_bytes);
void malloctor_free_impl(Allocator* a, void* head);

Mallocator::Mallocator() : Allocator{mallocator_growth_sug_impl, mallocator_alloc_impl, malloctor_free_impl} {}

size_t mallocator_growth_sug_impl(const Allocator* a, size_t count) {
    return max(size_t{CXB_MALLOCATOR_MIN_CAP}, CXB_MALLOCATOR_GROW_FN(count));
}

void* mallocator_alloc_impl(Allocator* a, bool fill_zeros, void* head, size_t num_bytes, size_t alignment, size_t old_n_bytes) {
    // TODO: alignment
    if(old_n_bytes > 0) {
        REQUIRES(head != nullptr);
        REQUIRES(num_bytes > old_n_bytes);

        void* data = realloc(head, num_bytes);
        if(fill_zeros) {
            if(!data) {
                data = calloc(num_bytes, 1);
                memcpy(data, head, old_n_bytes);
            } else {
                memset((char*)(data) + old_n_bytes, 0, num_bytes - old_n_bytes);
            }

        } else {
            if(!data) {
                data = malloc(num_bytes);
                memcpy(data, head, old_n_bytes);
            }
        }

        return data;
    } else {
        REQUIRES(head == nullptr);

        if(fill_zeros) {
            return calloc(num_bytes, 1);
        }
        return malloc(num_bytes);
    }
}

void malloctor_free_impl(Allocator* a, void* head) {
    free(head);
}

// TODO
Arena::Arena() {}

CXB_NS_END
