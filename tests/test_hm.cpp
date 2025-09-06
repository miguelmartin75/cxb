#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

size_t hash(const int& x);
#include <cxb/cxb.h>

size_t hash(const int& x) {
    return x; // TODO
}

TEST_CASE("basic", "[HashMap]") {
    Arena* a = get_perm();
    HashMap<int, int> kvs;
    kvs.put(a, {1, 2});
    kvs.extend(a, {
        {7, 9},
        {3, 5},
        {11, 9},
    });
    REQUIRE(kvs.contains(1));
    REQUIRE(kvs.contains(7));
    REQUIRE(kvs.contains(3));
    REQUIRE(kvs.contains(11));
    REQUIRE(!kvs.contains(2));
};
