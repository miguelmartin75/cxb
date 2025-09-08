#include "examples/memfile.h"

#include <cxb/cxb.h>

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    String8 in = "../cxb/cxb-cxx.h"_s8;
    String8 out = "../cxb/cxb-c.h"_s8;

    Arena* arena = get_perm();
    auto in_f = open_file(arena, in);
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
    } state = NONE;

    String8 in_data_str = in_f.value.data.as_string8();
    auto line_iter = in_data_str.split("\n"_s8);

    String8 line;
    println(out_f, "#pragma once\n");
    while(line_iter.next(line)) {
        if(line.starts_with("CXB_C_IMPORT"_s8)) {
            println("line={}, state={}", line, (int) state);
            println(out_f, "{}", line.trim_all("CXB_C_TYPE"_s8).trim(" "_s8));
        } else if(line.starts_with("CXB_C_TYPE"_s8)) {
            scope = SCOPE_TYPE;
            println(out_f, "{}", line.trim_all("CXB_C_TYPE"_s8).trim(" "_s8));
            continue;
        } else if(line.starts_with("};"_s8)) {
            if(scope == SCOPE_TYPE) {
                scope = SCOPE_GLOBAL;
                println(out_f, "{}", line.trim(" "_s8));
            }
        } else if(line.starts_with("CXB_C_COMPAT_BEGIN"_s8)) {
            if(state == STATE_IN_COMPAT_BLOCK) {
                println(stderr, "nested CXB_C_COMPAT_BEGIN / CXB_C_COMPAT_END block not supported");
                return 3;
            }
            if(line != "CXB_C_COMPAT_BEGIN"_s8) {
                println(stderr, "CXB_C_COMPAT_BEGIN should be on a seperate line");
                return 3;
            }
            bool has_any = false;
            while(line_iter.next(line) && !line.starts_with("CXB_C_COMPAT_END"_s8)) {
                println(out_f, "{}", line);
                has_any = true;
            }
            if(has_any) {
                println(out_f, "");
                (void) line_iter.next(line);
            } else {
                println(stderr, "CXB_C_COMPAT_BEGIN / CXB_C_COMPAT_END has no content");
                return 4;
            }
        }

        // go to end of type decl
        if(state == TYPE_SCOPE) {
            while(line_iter.next(line) && !line.starts_with("};"_s8)) {
                continue;
            }
            state = GLOBAL_SCOPE;
        }
    }

    return 0;
}

#include "examples/memfile.cpp"
