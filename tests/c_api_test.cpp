#include "cxb/cxb.h"
#include "c_api_test.h"

CXB_C_EXPORT MString join_paths(StringSlice p1, StringSlice p2, Allocator* alloc) {
    if(alloc == nullptr) {
        alloc = &default_alloc;
    }

    MString result{.data = nullptr, .len = 0, .null_term = true, .capacity = 0, .allocator = alloc};
    result.reserve(p1.len + p2.len + 1);
    result.extend(p1);
    result.push_back('/');
    result.extend(p2);
    return result;
}