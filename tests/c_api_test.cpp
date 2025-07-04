#include "c_api_test.h"

CXB_C_EXPORT MString join_paths(StringSlice p1, StringSlice p2, Allocator* alloc) {
    if(alloc == nullptr) {
        alloc = &default_alloc;
    }

    size_t new_len = p1.len + 1 + p2.len;

    MString result{.data = nullptr, .len = 0, .null_term = true, .allocator = alloc};
    result.reserve(new_len + 1);

    if(p1.len > 0) {
        memcpy(result.data, p1.data, p1.len);
    }

    result.data[p1.len] = '/';

    if(p2.len > 0) {
        memcpy(result.data + p1.len + 1, p2.data, p2.len);
    }

    result.data[new_len] = '\0';
    result.len = new_len;
    result.null_term = true;

    return result;
} 