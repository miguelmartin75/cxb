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

    xs.extend(arena, {20, 30, 50, 80});
    REQUIRE((void*) (arena->start + arena->pos) == (void*) (xs.data + xs.len));

    REQUIRE(xs.len == 5);
}

TEST_CASE("String8 arena member functions", "[String8][Arena]") {
    Arena* arena = arena_make_nbytes(KB(4));

    String8 s = arena_push_string8(arena, S8_LIT("abc"));

    s.resize(arena, 5, 'x');
    REQUIRE(s == S8_LIT("abcxx"));
    s.resize(arena, 3);
    REQUIRE(s == S8_LIT("abc"));

    s.push_back(arena, 'd');
    REQUIRE(s == S8_LIT("abcd"));
    s.pop_back(arena);
    REQUIRE(s == S8_LIT("abc"));

    s.insert(arena, 'X', 1);
    REQUIRE(s == S8_LIT("aXbc"));
    s.insert(arena, S8_LIT("YZ"), 2);
    REQUIRE(s == S8_LIT("aXYZbc"));

    s.extend(arena, S8_LIT("END"));
    REQUIRE(s == S8_LIT("aXYZbcEND"));

    s.pop_all(arena);
    REQUIRE(s.size() == 0);

    String8 num = arena_push_string8(arena, S8_LIT("1234"));
    auto pr = num.parse<i32>();
    REQUIRE(pr.exists);
    REQUIRE(pr.value == 1234);
    num.pop_all(arena);

    REQUIRE(arena->pos == sizeof(Arena) + 0);
}

TEST_CASE("Array arena member functions", "[Array][Arena]") {
    Arena* arena = arena_make_nbytes(KB(4));

    Array<int> arr{};
    arr.push_back(arena, 1);
    arr.push_back(arena, 2);
    REQUIRE(arr[0] == 1);
    REQUIRE(arr[1] == 2);

    arr.resize(arena, 4, 7);
    REQUIRE(arr[2] == 7);
    REQUIRE(arr[3] == 7);

    arr.resize(arena, 2);

    arr.insert(arena, 5, 1);
    REQUIRE(arr[1] == 5);

    arr.insert(arena, 8, 2);
    arr.insert(arena, 9, 3);
    REQUIRE(arr[2] == 8);
    REQUIRE(arr[3] == 9);
    REQUIRE(arr.size() == 5);

    REQUIRE((void*) (arr.data + arr.len) == (void*) (arena->start + arena->pos));
    arr.pop_back(arena);
    REQUIRE((void*) (arr.data + arr.len) == (void*) (arena->start + arena->pos));

    REQUIRE(arr.back() == 9);
    REQUIRE(arr.size() == 4);

    arr.pop_all(arena);
    REQUIRE(arr.size() == 0);
    REQUIRE(arena->pos == sizeof(Arena) + 0);
}
