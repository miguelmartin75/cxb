#include "c_api_test.h"

#include "cxb/cxb.h"

CXB_C_EXPORT void extend_elements(IntArray in_array) {
    auto arr = marray_from_pod<int>(in_array, &default_alloc);
    for(int i = 0; i < 10; ++i) {
        arr.push_back(i);
    }
}

CXB_C_EXPORT MString join_paths(StringSlice p1, StringSlice p2, Allocator* alloc) {
    if(alloc == nullptr) {
        alloc = &default_alloc;
    }

    MString result = MSTRING_NT(alloc);
    result.reserve(p1.len + p2.len + 1);
    result.extend(p1);
    if(result.back() != '/') {
        result.push_back('/');
    }
    result.extend(p2);
    return result;
}
