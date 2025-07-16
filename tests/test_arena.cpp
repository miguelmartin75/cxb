#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>

CXB_USE_NS;

struct Foo {
    int x, y;
};

TEST_CASE("push and pop", "[Arena]") {
    Arena arena = arena_make_nbytes(KB(64));
    Foo* foo = arena.push<Foo>(1);
    // arena.pop(foo);

    // auto foos = arena.push_array<Foo>(10);
    // REQUIRES(foos.len == 10);
    // arena.push_back(foos, Foo{3, 5});
}
