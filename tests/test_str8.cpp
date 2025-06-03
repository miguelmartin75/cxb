#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>
CXB_USE_NS;

TEST_CASE("Str8 default constructor", "[Str8]") {
    Str8 s;
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE(s.capacity() == CXB_MALLOCATOR_MIN_CAP);
    REQUIRE_FALSE(s.is_null_terminated());
}

TEST_CASE("Str8 from C string", "[Str8]") {
    const char* test_str = "Hello, World!";
    Str8 s(test_str);

    REQUIRE(s.size() == 13);
    REQUIRE_FALSE(s.empty());
    REQUIRE(s.is_null_terminated());
    REQUIRE(strcmp(s.c_str(), test_str) == 0);

    for(size_t i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == test_str[i]);
    }
}

TEST_CASE("Str8 from empty C string", "[Str8]") {
    Str8 s("");
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE(s.is_null_terminated());
    REQUIRE(strcmp(s.c_str(), "") == 0);
}

TEST_CASE("Str8 from null pointer", "[Str8]") {
    Str8 s(static_cast<const char*>(nullptr));
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE_FALSE(s.is_null_terminated());
}

TEST_CASE("Str8 from raw data", "[Str8]") {
    char data[] = {'H', 'e', 'l', 'l', 'o'};
    Str8 s(data, 5, false);

    REQUIRE(s.size() == 5);
    REQUIRE_FALSE(s.is_null_terminated());
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
        REQUIRE_FALSE(s.is_null_terminated());

        allocated_bytes = default_alloc.n_allocated_bytes;
    }
    REQUIRE(default_alloc.n_active_bytes == 0);
    REQUIRE(default_alloc.n_allocated_bytes == allocated_bytes);
}

TEST_CASE("Str8 push_back with null termination", "[Str8]") {
    Str8 s("Hello");
    REQUIRE(s.is_null_terminated());

    s.push_back('!');
    REQUIRE(s.size() == 6);
    REQUIRE(s.is_null_terminated());
    REQUIRE(strcmp(s.c_str(), "Hello!") == 0);
}

TEST_CASE("Str8 append C string", "[Str8]") {
    Str8 s("Hello");
    s.append(", World!");

    REQUIRE(s.size() == 13);
    REQUIRE(s.is_null_terminated());
    REQUIRE(strcmp(s.c_str(), "Hello, World!") == 0);
}

TEST_CASE("Str8 append other Str8", "[Str8]") {
    Str8 s1("Hello");
    Str8 s2(", World!");
    s1.append(s2);

    REQUIRE(s1.size() == 13);
    REQUIRE(s1.is_null_terminated());
    REQUIRE(strcmp(s1.c_str(), "Hello, World!") == 0);
}

TEST_CASE("Str8 append to non-null-terminated", "[Str8]") {
    Str8 s;
    s.push_back('H');
    s.push_back('i');
    REQUIRE_FALSE(s.is_null_terminated());

    s.append(" there");
    REQUIRE(s.size() == 8);
    REQUIRE_FALSE(s.is_null_terminated());

    // Content should be correct even without null termination
    const char expected[] = "Hi there";
    for(size_t i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == expected[i]);
    }
}

TEST_CASE("Str8 resize", "[Str8]") {
    Str8 s("Hello");
    s.resize(10, 'X');

    REQUIRE(s.size() == 10);
    REQUIRE(s.is_null_terminated());

    const char* expected = "HelloXXXXX";
    REQUIRE(strcmp(s.c_str(), expected) == 0);
}

TEST_CASE("Str8 resize shrinking", "[Str8]") {
    Str8 s("Hello, World!");
    s.resize(5);

    REQUIRE(s.size() == 5);
    REQUIRE(s.is_null_terminated());
    REQUIRE(strcmp(s.c_str(), "Hello") == 0);
}

TEST_CASE("Str8 pop_back", "[Str8]") {
    Str8 s("Hello");
    char c = s.pop_back();

    REQUIRE(c == 'o');
    REQUIRE(s.size() == 4);
    REQUIRE(s.is_null_terminated());
    REQUIRE(strcmp(s.c_str(), "Hell") == 0);
}

TEST_CASE("Str8 slice", "[Str8]") {
    Str8 s("Hello, World!");
    Str8 slice1 = s.slice(7);    // "World!"
    Str8 slice2 = s.slice(0, 5); // "Hello"

    REQUIRE(slice1.size() == 6);
    REQUIRE_FALSE(slice1.is_null_terminated());
    REQUIRE(slice1.allocator == nullptr); // slices don't own memory

    REQUIRE(slice2.size() == 5);
    REQUIRE_FALSE(slice2.is_null_terminated());

    // Check content
    const char world[] = "World!";
    const char hello[] = "Hello";
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
    REQUIRE(copy.is_null_terminated() == original.is_null_terminated());
    REQUIRE(copy.allocator == original.allocator);
    REQUIRE(copy.data != original.data); // different memory
    REQUIRE(strcmp(copy.c_str(), original.c_str()) == 0);
}

TEST_CASE("Str8 ensure_null_terminated", "[Str8]") {
    Str8 s;
    s.push_back('H');
    s.push_back('i');
    REQUIRE_FALSE(s.is_null_terminated());

    s.ensure_null_terminated();
    REQUIRE(s.is_null_terminated());
    REQUIRE(s.size() == 2);
    REQUIRE(strcmp(s.c_str(), "Hi") == 0);
}

TEST_CASE("Str8 remove_null_termination", "[Str8]") {
    Str8 s("Hello");
    REQUIRE(s.is_null_terminated());

    s.remove_null_termination();
    REQUIRE_FALSE(s.is_null_terminated());
    REQUIRE(s.size() == 5);
}

TEST_CASE("Str8 c_str() auto null-terminates", "[Str8]") {
    Str8 s;
    s.push_back('H');
    s.push_back('i');
    REQUIRE_FALSE(s.is_null_terminated());

    const char* cstr = s.c_str();
    REQUIRE(s.is_null_terminated()); // should now be null terminated
    REQUIRE(strcmp(cstr, "Hi") == 0);
}

TEST_CASE("Str8 memory management", "[Str8]") {
    size_t initial_active = default_alloc.n_active_bytes;
    size_t allocated_bytes = 0;

    {
        Str8 s;
        for(int i = 0; i < 100; ++i) {
            s.push_back('A' + (i % 26));
        }

        REQUIRE(s.size() == 100);
        allocated_bytes = default_alloc.n_allocated_bytes;
        REQUIRE(default_alloc.n_active_bytes > initial_active);
    }

    REQUIRE(default_alloc.n_active_bytes == initial_active);
    REQUIRE(default_alloc.n_allocated_bytes == allocated_bytes);
}

TEST_CASE("Str8 length encoding", "[Str8]") {
    Str8 s;

    // Test various lengths and null termination states
    s.set_len_and_null_terminated(0, false);
    REQUIRE(s.actual_len() == 0);
    REQUIRE_FALSE(s.is_null_terminated());

    s.set_len_and_null_terminated(0, true);
    REQUIRE(s.actual_len() == 0);
    REQUIRE(s.is_null_terminated());

    s.set_len_and_null_terminated(42, false);
    REQUIRE(s.actual_len() == 42);
    REQUIRE_FALSE(s.is_null_terminated());

    s.set_len_and_null_terminated(42, true);
    REQUIRE(s.actual_len() == 42);
    REQUIRE(s.is_null_terminated());

    // Test large length
    s.set_len_and_null_terminated(1000000, true);
    REQUIRE(s.actual_len() == 1000000);
    REQUIRE(s.is_null_terminated());
}

TEST_CASE("Str8 string literal macros", "[Str8]") {
    // Test s8_lit macro with string literal
    const char literal[] = "Hello, World!";
    Str8 s1 = s8_lit(literal);
    REQUIRE(s1.size() == 13);
    REQUIRE_FALSE(s1.is_null_terminated());
    REQUIRE(s1.allocator == nullptr); // slice doesn't own memory

    for(size_t i = 0; i < s1.size(); ++i) {
        REQUIRE(s1[i] == literal[i]);
    }

    // Test s8_cstr macro with C string
    const char* cstr = "Test string";
    Str8 s2 = s8_cstr(cstr);
    REQUIRE(s2.size() == 11);
    REQUIRE(s2.is_null_terminated());
    REQUIRE(s2.allocator == nullptr); // slice doesn't own memory
    REQUIRE(strcmp(s2.data, cstr) == 0);
}

TEST_CASE("Str8 null termination flag comprehensive behavior", "[Str8]") {
    // Test that null termination flag affects capacity planning
    Str8 s1("Hello"); // null-terminated from C string
    REQUIRE(s1.is_null_terminated());
    REQUIRE(s1.size() == 5);

    Str8 s2; // not null-terminated initially
    s2.push_back('H');
    s2.push_back('e');
    s2.push_back('l');
    s2.push_back('l');
    s2.push_back('o');
    REQUIRE_FALSE(s2.is_null_terminated());
    REQUIRE(s2.size() == 5);

    // When null-terminated, operations preserve the null terminator
    s1.push_back('!');
    REQUIRE(s1.is_null_terminated());
    REQUIRE(s1.size() == 6);
    REQUIRE(s1.data[6] == '\0'); // null terminator is maintained

    // When not null-terminated, no extra space is used
    s2.push_back('!');
    REQUIRE_FALSE(s2.is_null_terminated());
    REQUIRE(s2.size() == 6);

    // Converting between states
    s2.ensure_null_terminated();
    REQUIRE(s2.is_null_terminated());
    REQUIRE(s2.size() == 6);
    REQUIRE(s2.data[6] == '\0');

    s1.remove_null_termination();
    REQUIRE_FALSE(s1.is_null_terminated());
    REQUIRE(s1.size() == 6);

    // Test that resize preserves null termination state
    s1.ensure_null_terminated();
    s1.resize(10, 'X');
    REQUIRE(s1.is_null_terminated());
    REQUIRE(s1.size() == 10);
    REQUIRE(s1.data[10] == '\0');

    s2.remove_null_termination();
    s2.resize(8, 'Y');
    REQUIRE_FALSE(s2.is_null_terminated());
    REQUIRE(s2.size() == 8);
}