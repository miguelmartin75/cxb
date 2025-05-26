#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#define CXB_IMPL
#include "cxb.h"


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
