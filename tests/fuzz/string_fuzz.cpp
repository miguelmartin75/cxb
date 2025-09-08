#include "cxb/cxb-cxx.h"

#include <stddef.h>
#include <stdint.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    ArenaTmp scratch = begin_scratch();
    Arena* arena = scratch.arena;

    String8 input{.data = (char*) data, .len = size, .not_null_term = true};
    String8 str = arena_push_string8(arena, input);

    size_t idx = 0;
    while(idx < size) {
        u8 op = data[idx++] % 5;
        switch(op) {
            case 0: {
                if(idx < size) {
                    string8_push_back(str, arena, (char) data[idx++]);
                }
                break;
            }
            case 1: {
                if(idx < size) {
                    size_t max_len = size - idx;
                    u8 len_byte = data[idx];
                    idx += 1;
                    size_t len = len_byte % (max_len + 1);
                    String8 to_append{.data = (char*) data + idx, .len = len, .not_null_term = true};
                    string8_extend(str, arena, to_append);
                    idx += len;
                }
                break;
            }
            case 2: {
                if(str.len > 0) {
                    size_t i = data[idx % size] % (str.len + 1);
                    size_t j = data[(idx + 1) % size] % (str.len + 1);
                    String8 slice = str.slice((i64) i, (i64) j);
                    (void) slice;
                }
                break;
            }
            case 3: {
                ArenaTmp decode_scratch = begin_scratch();
                Array<u32> codepoints = decode_string8(decode_scratch.arena, str);
                (void) codepoints;
                end_scratch(decode_scratch);
                break;
            }
            case 4: {
                Utf8Iter iter = make_utf8_iter(str);
                Utf8IterBatch batch = {};
                while(iter.next(batch)) {}
                break;
            }
        }
    }

    end_scratch(scratch);
    return 0;
}
