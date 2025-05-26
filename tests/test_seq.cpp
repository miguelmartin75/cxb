#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include <cxb.h>
using namespace cxb;


TEST_CASE( "push_back", "[Seq]" ) {
    Seq<int> xs;
    REQUIRE(xs.len == 0);
    REQUIRE(xs.capacity() == 32);

    for(int i = 0; i < 256; ++i) {
        xs.push_back(i);
    }

    REQUIRE(xs.len == 256);
    for(int i = 0; i < 256; ++i) {
        REQUIRE(xs[i] == i);
    }
}

TEST_CASE( "copy", "[Seq]" ) {
    Seq<int> xs;
    xs.resize(64, 2);
    for(int i = 0; i < xs.len; ++i) {
        REQUIRE(xs[i] == 2);
    }

    Seq<int> view = xs;
    REQUIRE(view.allocator == nullptr);

    Seq<int> real_copy = xs.copy();
    REQUIRE(real_copy.allocator == xs.allocator);
    REQUIRE(real_copy.data != xs.data);
}
