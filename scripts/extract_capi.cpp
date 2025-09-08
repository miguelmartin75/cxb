#include <cxb/cxb.h>

#include "examples/memfile.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    String8 in = "../cxb/cxb-cxx.h"_s8;
    String8 out = "../cxb/cxb-c.h"_s8;

    Arena* arena = get_perm();
    auto in_f = open_file(arena, in);
    if(in_f) {
        println(stderr, "Error opening file {}, reason: {} (code={})", in, in_f.reason, (int)in_f.error);
        return 1;
    }
    FILE* out_f = fopen(out.c_str(), "w");

    String8 in_data_str = in_f.value.data.as_string8();
    for(String8 line : in_data_str.split("\n"_s8)) {
        if(line.startswith(
    }

    return 0;
}

#include "examples/memfile.cpp"
