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
    REQUIRE(kvs.contains(1));
};
