#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>

struct Item {
    int key;
    int id;
};

TEST_CASE("merge_sort sorts integers", "[algos]") {
    AArenaTmp tmp = begin_scratch();
    Arena* arena = tmp.arena;
    Array<int> xs = arena_push_array<int>(arena, 6);
    xs[0] = 5;
    xs[1] = 1;
    xs[2] = 4;
    xs[3] = 2;
    xs[4] = 3;
    xs[5] = 0;

    merge_sort(xs.data, xs.len);

    for(int i = 0; i < 6; ++i) {
        REQUIRE(xs[i] == i);
    }
}

TEST_CASE("merge_sort is stable", "[algos]") {
    AArenaTmp tmp = begin_scratch();
    Arena* arena = tmp.arena;
    Array<Item> xs = arena_push_array<Item>(arena, 4);
    xs[0] = {1, 0};
    xs[1] = {1, 1};
    xs[2] = {2, 2};
    xs[3] = {2, 3};

    merge_sort(xs.data, xs.len, [](const Item& a, const Item& b) { return a.key < b.key; });

    REQUIRE(xs[0].key == 1);
    REQUIRE(xs[1].key == 1);
    REQUIRE(xs[0].id == 0);
    REQUIRE(xs[1].id == 1);
    REQUIRE(xs[2].key == 2);
    REQUIRE(xs[3].key == 2);
    REQUIRE(xs[2].id == 2);
    REQUIRE(xs[3].id == 3);
}
