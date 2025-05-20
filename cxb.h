#pragma once

#define CXB_EXPORT

CXB_EXPORT int foo();

#ifdef CXB_IMPL
#include "cxb.cpp"
#endif
