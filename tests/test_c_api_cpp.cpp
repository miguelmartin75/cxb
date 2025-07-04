#include "c_api_test.h"
#include "cxb/cxb.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("join_paths -> MString in C++") {
    size_t mem_before = default_alloc.n_active_bytes;
    {
        StringSlice p1 = S8_LIT("foo");
        StringSlice p2 = S8_LIT("bar");

        MString joined = join_paths(p1, p2, &default_alloc);
        REQUIRE(joined == S8_LIT("foo/bar"));

        joined.destroy();
    }
    REQUIRE(default_alloc.n_active_bytes == mem_before);
}

TEST_CASE("join_paths -> String in C++") {
    size_t mem_before = default_alloc.n_active_bytes;
    {
        StringSlice p1 = S8_LIT("foo");
        StringSlice p2 = S8_LIT("bar");

        String joined = join_paths(p1, p2, &default_alloc);
        REQUIRE(joined == S8_LIT("foo/bar"));
    }
    REQUIRE(default_alloc.n_active_bytes == mem_before);
}