#include "examples/memfile.h"

#include <cxb/cxb.h>

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    String8 in = "../cxb/cxb-cxx.h"_s8;
    String8 out = "../cxb/cxb-c.h"_s8;

    Arena* arena = get_perm();
    auto in_f = open_memfile(arena, in);
    if(in_f) {
        println(stderr, "Error opening file {}, reason: {} (code={})", in, in_f.reason, (int) in_f.error);
        return 1;
    }
    FILE* out_f = fopen(out.c_str(), "w");

    enum Scope {
        SCOPE_GLOBAL,
        SCOPE_TYPE,
    } scope = SCOPE_GLOBAL;

    enum State {
        STATE_NONE,
        STATE_IN_COMPAT_BLOCK,
    } state = STATE_NONE;

    String8 in_data_str = in_f.value.data.as_string8();
    String8SplitIterator line_iter = in_data_str.split("\n"_s8);

    String8 line;
    println(out_f, "#pragma once\n");
    while(line_iter.next(line)) {
        if(line.starts_with("CXB_C_IMPORT"_s8)) {
            println(out_f, "{}", line.trim_all_left("CXB_C_IMPORT"_s8).trim(" "_s8));
        } else if(line.starts_with("CXB_C_TYPE"_s8)) {
            scope = SCOPE_TYPE;
            println(out_f, "\n{}", line.trim_all_left("CXB_C_TYPE"_s8).trim(" "_s8));
        } else if(line.starts_with("};"_s8)) {
            if(scope == SCOPE_TYPE) {
                scope = SCOPE_GLOBAL;
                println(out_f, "{}", line.trim(" "_s8));
            }
        } else if(line.trim_left(" "_s8).starts_with("CXB_C_COMPAT_END"_s8)) {
            if(state != STATE_IN_COMPAT_BLOCK) {
                println(stderr, "CXB_C_COMPAT_END must be before CXB_C_COMPAT_BEGIN");
                return 3;
            }
            state = STATE_NONE;
        } else if(line.trim_left(" "_s8).starts_with("CXB_C_COMPAT_BEGIN"_s8)) {
            if(state == STATE_IN_COMPAT_BLOCK) {
                println(stderr, "nested CXB_C_COMPAT_BEGIN / CXB_C_COMPAT_END block not supported");
                return 3;
            }
            state = STATE_IN_COMPAT_BLOCK;
        } else if(state == STATE_IN_COMPAT_BLOCK) {
            println(out_f, "{}", line);
        }
    }

    return 0;
}

#include "examples/memfile.cpp"
