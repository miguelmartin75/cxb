#include <stdio.h>
#include "examples/parser.h"

int main(int argc, char* argv[]) {
    if(argc == 1) {
        fprintf(stderr, "expected input file, usage:\n%s <input-file>\n", argv[0]);
        return 1;
    }
    Module* mod = module_make(S8_LIT("main"), nullptr, nullptr);

    ParseFileResult parse_result = module_parse_file(mod, S8_CSTR(argv[1]));
    if(!parse_result) {
        fprintf(stderr, "Failed to parse: %s", argv[1]);
        return 1;
    }
    return 0;
}
