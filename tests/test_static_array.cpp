#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>
#include <type_traits>

TEST_CASE("make_static_array conversion", "StaticArray") {
    auto sa = make_static_array<int>({1, 2, 3});
    Array<int> xs{sa};
    REQUIRE(xs.len == 3);
    REQUIRE(xs[0] == 1);
    REQUIRE(xs[1] == 2);
    REQUIRE(xs[2] == 3);

    static_assert(!std::is_constructible_v<Array<int>, decltype(make_static_array<int>({1, 2, 3}))>);
}
