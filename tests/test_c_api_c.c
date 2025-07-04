#include "c_api_test.h"

#include <string.h>

int main(void) {
    StringSlice p1 = {.data = "foo", .len = 3, .null_term = 1};
    StringSlice p2 = {.data = "bar", .len = 3, .null_term = 1};

    MString result = join_paths(p1, p2, &default_alloc);

    REQUIRES(result.len == 7);
    REQUIRES(result.null_term == 1);
    int ret = (strcmp(result.data, "foo/bar") == 0) ? 0 : 1;
    REQUIRES(ret == 0);
    return ret;
}