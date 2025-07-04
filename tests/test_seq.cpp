#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>
CXB_USE_NS;

TEST_CASE("push_back", "[Seq]") {
    size_t allocated_bytes = 0;
    {
        Seq<int> xs;
        REQUIRE(xs.len == 0);
        REQUIRE(xs.capacity() == CXB_MALLOCATOR_MIN_CAP);

        for(int i = 0; i < 256; ++i) {
            xs.push_back(i);
        }
        size_t active_bytes = default_alloc.n_active_bytes;
        allocated_bytes = default_alloc.n_allocated_bytes;
        REQUIRE(active_bytes == allocated_bytes);

        REQUIRE(xs.len == 256);
        for(int i = 0; i < 256; ++i) {
            REQUIRE(xs[i] == i);
        }
    }
    REQUIRE(default_alloc.n_active_bytes == 0);
    REQUIRE(default_alloc.n_allocated_bytes == allocated_bytes);
}

TEST_CASE("copy", "[Seq]") {
    Seq<int> xs;
    xs.resize(64, 2);
    for(int i = 0; i < xs.len; ++i) {
        REQUIRE(xs[i] == 2);
    }

    Seq<int> view = xs.slice();
    REQUIRE(view.allocator == nullptr);

    Seq<int> real_copy = xs.copy();
    REQUIRE(real_copy.allocator == xs.allocator);
    REQUIRE(real_copy.data != xs.data);
}
