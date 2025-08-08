#pragma once

#include "cxb/cxb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IntArray {
    int* data;
    size_t len;
    size_t capacity;
    Allocator* allocator;
} IntArray;

CXB_C_EXPORT void extend_elements(IntArray* in_array);

CXB_C_EXPORT MString join_paths(String8 p1, String8 p2, Allocator* alloc);

#ifdef __cplusplus
} // extern "C"
#endif
