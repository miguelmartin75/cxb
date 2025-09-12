#include "cxb/cxb-cxx.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if(size < sizeof(f64)) {
        return 0;
    }

    ArenaTmp scratch = begin_scratch();
    Arena* arena = scratch.arena;

    f64 value = 0;
    memcpy(&value, data, sizeof(f64));
    String8 args{.data = (char*) data + sizeof(f64), .len = size - sizeof(f64), .not_null_term = true};

    String8 dst = arena_push_string8(arena, 1);
    dst.len = 0;

    format_value(arena, dst, args, value);
    string8_pop_all(dst, arena);
    format_value(arena, dst, args, (f32) value);

    end_scratch(scratch);
    return 0;
}
