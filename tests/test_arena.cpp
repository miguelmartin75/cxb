#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>

struct Foo {
    int x, y;
};

TEST_CASE("push and pop", "[Arena*]") {
    Arena* arena = arena_make_nbytes(KB(4));
    REQUIRE(arena->end - arena->start == KB(4));

    Foo* foo = push<Foo>(arena, 1);
    pop(arena, foo);
    REQUIRE(arena->pos == sizeof(Arena) + 0);

    auto foos = push_array<Foo>(arena, 10);
    REQUIRE(foos.len == 10);
    push_back(arena, foos, Foo{3, 5});

    REQUIRE(foos.len == 11);
    REQUIRE(foos.back().x == 3);
    REQUIRE(foos.back().y == 5);

    pop_all(arena, foos);
    REQUIRE(arena->pos == sizeof(Arena) + 0);
}

TEST_CASE("string push/pop", "[Arena*]") {
    Arena* arena = arena_make_nbytes(KB(4));
    REQUIRE(arena->end - arena->start == KB(4));

    String8 str = push_str(arena);
    push_back(arena, str, 'a');
    insert(arena, str, 'b', 0);
    REQUIRE(str == S8_LIT("ba"));
    REQUIRE(str.n_bytes() == 3);
    REQUIRE(arena->pos == sizeof(Arena) + str.n_bytes());
}

TEST_CASE("string insert", "[Arena*]") {
    Arena* arena = arena_make_nbytes(KB(4));
    REQUIRE(arena->end - arena->start == KB(4));

    String8 str = push_str(arena);
    push_back(arena, str, 'a');
    insert(arena, str, 'b', 0);
    REQUIRE(str == S8_LIT("ba"));
    REQUIRE(str.n_bytes() == 3);
    REQUIRE(arena->pos == sizeof(Arena) + str.n_bytes());

    pop_all(arena, str);
    REQUIRE(arena->pos == sizeof(Arena) + 0);
}

TEST_CASE("string extend", "[Arena*]") {
    Arena* arena = arena_make_nbytes(KB(4));
    REQUIRE(arena->end - arena->start == KB(4));

    String8 str = push_str(arena, S8_LIT("abc"));
    extend(arena, str, S8_LIT("def"));
    REQUIRE(str == S8_LIT("abcdef"));
    REQUIRE(str.n_bytes() == 7);
    REQUIRE(arena->pos == sizeof(Arena) + str.n_bytes());

    insert(arena, str, S8_LIT("middle"), 2);
    REQUIRE(str == S8_LIT("abmiddlecdef"));
    REQUIRE(str.n_bytes() == 13);
    REQUIRE(arena->pos == sizeof(Arena) + 13);

    pop_all(arena, str);
    REQUIRE(arena->pos == sizeof(Arena) + 0);
}
