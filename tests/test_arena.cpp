#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>

struct Foo {
    int x, y;
};

TEST_CASE("push and pop", "[Arena]") {
    Arena* arena = arena_make_nbytes(KB(4));
    REQUIRE(arena->end - arena->start == KB(4));

    Foo* foo = arena_push<Foo>(arena, 1);
    arena_pop(arena, foo);
    REQUIRE(arena->pos == sizeof(Arena) + 0);

    auto foos = arena_push_array<Foo>(arena, 10);
    REQUIRE(foos.len == 10);
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (foos.data + foos.len));
    array_push_back(foos, arena, Foo{3, 5});
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (foos.data + foos.len));

    REQUIRE(foos.len == 11);
    REQUIRE(foos.back().x == 3);
    REQUIRE(foos.back().y == 5);

    array_pop_all(foos, arena);
    REQUIRE(arena->pos == sizeof(Arena) + 0);
}

TEST_CASE("string push/pop", "[Arena]") {
    Arena* arena = arena_make_nbytes(KB(4));
    REQUIRE(arena->end - arena->start == KB(4));

    String8 str = arena_push_string8(arena);
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (str.data + str.n_bytes()));
    string8_push_back(str, arena, 'a');
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (str.data + str.n_bytes()));
    string8_insert(str, arena, 'b', 0);
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (str.data + str.n_bytes()));
    REQUIRE(str == S8_LIT("ba"));
    REQUIRE(str.n_bytes() == 3);
}

TEST_CASE("string insert", "[Arena]") {
    Arena* arena = arena_make_nbytes(KB(4));
    REQUIRE(arena->end - arena->start == KB(4));

    String8 str = arena_push_string8(arena);
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (str.data + str.n_bytes()));
    string8_push_back(str, arena, 'a');
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (str.data + str.n_bytes()));
    string8_insert(str, arena, 'b', 0);
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (str.data + str.n_bytes()));

    REQUIRE(str == S8_LIT("ba"));
    REQUIRE(str.n_bytes() == 3);
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (str.data + str.n_bytes()));

    string8_pop_all(str, arena);
    REQUIRE(arena->pos == sizeof(Arena) + 0);
}

TEST_CASE("string extend", "[Arena]") {
    Arena* arena = arena_make_nbytes(KB(4));
    REQUIRE(arena->end - arena->start == KB(4));

    String8 str = arena_push_string8(arena, S8_LIT("abc"));
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (str.data + str.n_bytes()));
    string8_extend(str, arena, S8_LIT("def"));
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (str.data + str.n_bytes()));

    REQUIRE(str == S8_LIT("abcdef"));
    REQUIRE(str.n_bytes() == 7);

    string8_insert(str, arena, S8_LIT("middle"), 2);
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (str.data + str.n_bytes()));
    REQUIRE(str == S8_LIT("abmiddlecdef"));
    REQUIRE(str.n_bytes() == 13);

    string8_pop_all(str, arena);
    REQUIRE(arena->pos == sizeof(Arena) + 0);
}

TEST_CASE("array insert", "[Arena]") {
    Arena* arena = arena_make_nbytes(KB(4));
    REQUIRE(arena->end - arena->start == KB(4));

    Array<int> xs = {};
    xs.push_back(arena, 10);
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (xs.data + xs.len));

    int ys[] = {20, 30, 50, 80};
    xs.extend(arena, Array<int>(ys, 4));
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (xs.data + xs.len));

    REQUIRE(xs.len == 5);
}
