#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>
CXB_USE_NS;

TEST_CASE("Str8 default constructor", "[Str8]") {
    Str8 s;
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE(s.capacity() == CXB_MALLOCATOR_MIN_CAP);
    REQUIRE_FALSE(s.null_term);
}

TEST_CASE("Str8 from C string", "[Str8]") {
    const char* test_str = "Hello, World!";
    Str8 s(test_str);

    REQUIRE(s.size() == 13);
    REQUIRE_FALSE(s.empty());
    REQUIRE(s.null_term);
    REQUIRE(strcmp(s.c_str(), test_str) == 0);

    for(size_t i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == test_str[i]);
    }
}

TEST_CASE("Str8 from empty C string", "[Str8]") {
    Str8 s("");
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE(s.null_term);
    REQUIRE(s == s8_lit(""));
}

TEST_CASE("Str8 from null pointer", "[Str8]") {
    Str8 s(static_cast<const char*>(nullptr));
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE(s.null_term);
}

TEST_CASE("Str8 from raw data", "[Str8]") {
    char data[] = {'H', 'e', 'l', 'l', 'o'};
    Str8 s(data, 5, nullptr);

    REQUIRE(s.size() == 5);
    REQUIRE(s.null_term);
    REQUIRE(s.data == data);

    for(size_t i = 0; i < 5; ++i) {
        REQUIRE(s[i] == data[i]);
    }
}

TEST_CASE("Str8 push_back", "[Str8]") {
    size_t allocated_bytes = 0;
    {
        Str8 s;
        s.push_back('H');
        s.push_back('i');

        REQUIRE(s.size() == 2);
        REQUIRE(s[0] == 'H');
        REQUIRE(s[1] == 'i');
        REQUIRE_FALSE(s.null_term);

        allocated_bytes = default_alloc.n_allocated_bytes;
    }
    REQUIRE(default_alloc.n_active_bytes == 0);
    REQUIRE(default_alloc.n_allocated_bytes == allocated_bytes);
}

TEST_CASE("Str8 push_back with null termination", "[Str8]") {
    Str8 s("Hello");
    REQUIRE(s.null_term);
    REQUIRE(s.allocator);
    REQUIRE(s.len == 5);
    REQUIRE(s.size() == 5);

    s.push_back('!');
    REQUIRE(s.size() == 6);
    REQUIRE(s.null_term);

    Str8 cmp = s8_lit("Hello!");
    for(int i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == cmp[i]);
    }
    REQUIRE(s == cmp);
    REQUIRE(strcmp(s.c_str(), "Hello!") == 0);
}

TEST_CASE("Str8 append C string", "[Str8]") {
    Str8 s("Hello");
    s.extend(", World!");

    REQUIRE(s.len == 13);
    REQUIRE(s.null_term);
    REQUIRE(s == s8_lit("Hello, World!"));
    REQUIRE(strcmp(s.c_str(), "Hello, World!") == 0);
}

TEST_CASE("Str8 append other Str8", "[Str8]") {
    Str8 s1("Hello");
    Str8 s2(", World!");
    s1.extend(s2);

    REQUIRE(s1.len == 13);
    REQUIRE(s1.null_term);
    REQUIRE(s1 == s8_lit("Hello, World!"));
}

TEST_CASE("Str8 append to non-null-terminated", "[Str8]") {
    Str8 s;
    s.push_back('H');
    s.push_back('i');
    REQUIRE_FALSE(s.null_term);

    s.extend(" there");
    REQUIRE(s.size() == 8);
    REQUIRE_FALSE(s.null_term);

    const char expected[] = "Hi there";
    for(size_t i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == expected[i]);
    }
    REQUIRE(s == s8_lit("Hi there"));
}

TEST_CASE("Str8 resize", "[Str8]") {
    Str8 s("Hello");
    s.resize(10, 'X');

    REQUIRE(s.size() == 10);
    REQUIRE(s.null_term);

    REQUIRE(s == s8_lit("HelloXXXXX"));
}

TEST_CASE("Str8 resize shrinking", "[Str8]") {
    Str8 s("Hello, World!");
    s.resize(5);

    REQUIRE(s.size() == 5);
    REQUIRE(s.null_term);
    REQUIRE(s == s8_lit("Hello"));
}

TEST_CASE("Str8 pop_back", "[Str8]") {
    Str8 s("Hello");
    char c = s.pop_back();

    REQUIRE(c == 'o');
    REQUIRE(s.size() == 4);
    REQUIRE(s.null_term);
    REQUIRE(s == s8_lit("Hell"));
}

TEST_CASE("Str8 slice", "[Str8]") {
    Str8 s("Hello, World!");
    Str8 slice1 = s.slice(7);    // "World!"
    Str8 slice2 = s.slice(0, 5); // "Hello"

    REQUIRE(slice1.size() == 6);
    REQUIRE(slice1.null_term);
    REQUIRE(slice1.allocator == nullptr);

    REQUIRE(slice2.size() == 5);
    REQUIRE_FALSE(slice2.null_term);

    const char hello[] = "Hello";
    const char world[] = "World!";
    for(size_t i = 0; i < slice1.size(); ++i) {
        REQUIRE(slice1[i] == world[i]);
    }
    for(size_t i = 0; i < slice2.size(); ++i) {
        REQUIRE(slice2[i] == hello[i]);
    }
}

TEST_CASE("Str8 copy", "[Str8]") {
    Str8 original("Hello, World!");
    Str8 copy = original.copy();

    REQUIRE(copy.size() == original.size());
    REQUIRE(copy.null_term == original.null_term);
    REQUIRE(copy.allocator == original.allocator);
    REQUIRE(copy.data != original.data); // different memory
    REQUIRE(original == copy);
}

TEST_CASE("Str8 ensure_null_terminated", "[Str8]") {
    Str8 s;
    s.push_back('H');
    s.push_back('i');
    REQUIRE_FALSE(s.null_term);

    s.ensure_null_terminated();
    REQUIRE(s.null_term);
    REQUIRE(s.size() == 2);
    REQUIRE(s == s8_lit("Hi"));
}
