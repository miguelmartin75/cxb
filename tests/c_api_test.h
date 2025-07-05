#pragma once

#include "cxb/cxb.h"

#ifdef __cplusplus
extern "C" {
#endif

CXB_C_EXPORT MString join_paths(StringSlice p1, StringSlice p2, Allocator* alloc);

#ifdef __cplusplus
} // extern "C"
#endif