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
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
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
    REQUIRE(default_alloc.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("nested_seq", "[Seq]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        Seq<Seq<int>> nested;
        REQUIRE(nested.len == 0);
        REQUIRE(nested.capacity() == CXB_MALLOCATOR_MIN_CAP);

        for(int i = 0; i < 10; ++i) {
            Seq<int> inner;
            for(int j = 0; j < i + 1; ++j) {
                inner.push_back(i * 10 + j);
            }
            nested.push_back(::move(inner));
        }

        REQUIRE(nested.len == 10);

        for(int i = 0; i < nested.len; ++i) {
            REQUIRE(nested[i].len == i + 1);
            for(int j = 0; j < nested[i].len; ++j) {
                REQUIRE(nested[i][j] == i * 10 + j);
            }
        }

        nested[5].push_back(999);
        REQUIRE(nested[5].len == 7);
        REQUIRE(nested[5][6] == 999);

        Seq<int> new_inner;
        for(int k = 0; k < 5; ++k) {
            new_inner.push_back(k * 100);
        }
        nested.push_back(move(new_inner));

        REQUIRE(nested.len == 11);
        REQUIRE(nested[10].len == 5);
        REQUIRE(nested[10][0] == 0);
        REQUIRE(nested[10][4] == 400);

        REQUIRE(default_alloc.n_active_bytes > allocated_bytes_before);
    }

    REQUIRE(default_alloc.n_active_bytes == allocated_bytes_before);
}
