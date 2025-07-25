#include "c_api_test.h"

#include <string.h>

int main(void) {
    StringSlice p1 = {.data = "foo", .len = 3, .null_term = 1};
    StringSlice p2 = {.data = "bar", .len = 3, .null_term = 1};

    MString result = join_paths(p1, p2, &heap_alloc);

    ASSERT(result.len == 7);
    ASSERT(result.null_term == 1);
    int ret = (strcmp(result.data, "foo/bar") == 0) ? 0 : 1;
    ASSERT(ret == 0);

    StringSlice res_slice = {.data = result.data, .len = result.len, .null_term = result.null_term};

    StringSlice slice_foo = cxb_ss_slice(res_slice, 0, 1);
    ASSERT(cxb_ss_size(slice_foo) == 2);
    ASSERT(!cxb_ss_empty(slice_foo));

    StringSlice slice_bar = cxb_ss_slice(res_slice, 3, -1);
    ASSERT(cxb_ss_n_bytes(slice_bar) == 4 + 1);

    cxb_mstring_destroy(&result);

    ASSERT(result.data == NULL);

    IntArray arr = {};
    extend_elements(&arr);
    ASSERT(arr.len == 10);
    ASSERT(arr.allocator != NULL);
    for(int i = 0; i < 10; ++i) {
        ASSERT(arr.data[i] == i);
    }
    return ret;
}
