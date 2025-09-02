#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>

TEST_CASE("String8 from C string", "[String8]") {
    const char* test_str = "Hello, World!";
    String8 s{.data = const_cast<char*>(test_str), .len = 13, .not_null_term = false};

    REQUIRE(s.size() == 13);
    REQUIRE_FALSE(s.empty());
    REQUIRE_FALSE(s.not_null_term);
    REQUIRE(strcmp(s.c_str(), test_str) == 0);

    for(size_t i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == test_str[i]);
    }
}

TEST_CASE("String8 from empty C string", "[String8]") {
    String8 s = ""_s8;
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE_FALSE(s.not_null_term);
    REQUIRE(s == S8_LIT(""));
}

TEST_CASE("String8 from raw data", "[String8]") {
    char data[] = {'H', 'e', 'l', 'l', 'o'};
    String8 s{.data = data, .len = 5, .not_null_term = false};

    REQUIRE(s.size() == 5);
    REQUIRE_FALSE(s.not_null_term);
    REQUIRE(s.data == data);

    for(size_t i = 0; i < 5; ++i) {
        REQUIRE(s[i] == data[i]);
    }
}

TEST_CASE("String push_back", "[String]") {
    i64 allocated_bytes = 0;
    {
        AString8 s;
        s.push_back('H');
        s.push_back('i');

        REQUIRE(s.size() == 2);
        REQUIRE(s[0] == 'H');
        REQUIRE(s[1] == 'i');
        REQUIRE_FALSE(s.not_null_term);

        allocated_bytes = heap_alloc_data.n_allocated_bytes;
    }
    REQUIRE(heap_alloc_data.n_active_bytes == 0);
    REQUIRE(heap_alloc_data.n_allocated_bytes == allocated_bytes);
}

TEST_CASE("String8 push_back with null termination", "[String]") {
    AString8 s("Hello");
    REQUIRE_FALSE(s.not_null_term);
    REQUIRE(s.allocator);
    REQUIRE(s.len == 5);
    REQUIRE(s.size() == 5);

    s.push_back('!');
    REQUIRE(s.size() == 6);
    REQUIRE_FALSE(s.not_null_term);

    String8 cmp = S8_LIT("Hello!");
    for(u64 i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == cmp[i]);
    }
    REQUIRE(s == cmp);
    REQUIRE(strcmp(s.c_str(), "Hello!") == 0);
}

TEST_CASE("String8 append C string", "[String8]") {
    AString8 s("Hello");
    s.extend(", World!");

    REQUIRE(s.len == 13);
    REQUIRE_FALSE(s.not_null_term);
    REQUIRE(s == S8_LIT("Hello, World!"));
    REQUIRE(strcmp(s.c_str(), "Hello, World!") == 0);
}

TEST_CASE("String8 append other String8", "[String8]") {
    AString8 s1("Hello");
    String8 s2 = S8_LIT(", World!");
    s1.extend(s2);

    REQUIRE(s1.len == 13);
    REQUIRE_FALSE(s1.not_null_term);
    REQUIRE(s1 == S8_LIT("Hello, World!"));
}

TEST_CASE("String8 append to non-null-terminated", "[String8]") {
    AString8 s;
    s.push_back('H');
    s.push_back('i');
    REQUIRE_FALSE(s.not_null_term);

    s.extend(" there");
    REQUIRE(s.size() == 8);
    REQUIRE_FALSE(s.not_null_term);

    const char expected[] = "Hi there";
    for(size_t i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == expected[i]);
    }
    REQUIRE(s == S8_LIT("Hi there"));
}

TEST_CASE("String8 resize", "[String8]") {
    AString8 s("Hello");
    s.resize(10, 'X');

    REQUIRE(s.size() == 10);
    REQUIRE_FALSE(s.not_null_term);

    REQUIRE(s == S8_LIT("HelloXXXXX"));
}

TEST_CASE("String8 resize shrinking", "[String8]") {
    AString8 s("Hello, World!");
    s.resize(5);

    REQUIRE(s.size() == 5);
    REQUIRE_FALSE(s.not_null_term);
    REQUIRE(s == S8_LIT("Hello"));
}

TEST_CASE("String8 pop_back", "[String8]") {
    AString8 s("Hello");
    char c = s.pop_back();

    REQUIRE(c == 'o');
    REQUIRE(s.size() == 4);
    REQUIRE_FALSE(s.not_null_term);
    REQUIRE(s == S8_LIT("Hell"));
}

TEST_CASE("String8 slice", "[String8]") {
    String8 s = S8_LIT("Hello, World!");
    String8 slice1 = s.slice(7);    // "World!"
    String8 slice2 = s.slice(0, 4); // "Hello"

    REQUIRE(slice1.size() == 6);
    REQUIRE_FALSE(slice1.not_null_term);

    REQUIRE(slice2.size() == 5);
    REQUIRE(slice2.not_null_term);

    const char hello[] = "Hello";
    const char world[] = "World!";
    for(size_t i = 0; i < slice1.size(); ++i) {
        REQUIRE(slice1[i] == world[i]);
    }
    for(size_t i = 0; i < slice2.size(); ++i) {
        REQUIRE(slice2[i] == hello[i]);
    }
}

TEST_CASE("String8 copy", "[String8]") {
    AString8 original("Hello, World!");
    AString8 copy = original.copy();

    REQUIRE(copy.size() == original.size());
    REQUIRE(copy.not_null_term == original.not_null_term);
    REQUIRE(copy.allocator == original.allocator);
    REQUIRE(copy.data != original.data); // different memory
    REQUIRE(original == copy);
}

TEST_CASE("String8 ensure_not_null_terminated", "[String8]") {
    AString8 s = {};
    s.push_back('H');
    s.push_back('i');
    REQUIRE_FALSE(s.not_null_term);

    s.ensure_not_null_terminated();
    REQUIRE_FALSE(s.not_null_term);
    REQUIRE(s.size() == 2);
    REQUIRE(s == S8_LIT("Hi"));
}

TEST_CASE("Utf8Iterator with ASCII string", "[Utf8Iterator]") {
    String8 s = S8_LIT("Hello World");
    ArenaTmp scratch = begin_scratch();

    Array<u32> codepoints = decode_string8(scratch.arena, s);
    REQUIRE(codepoints.len == s.len);
    for(u64 i = 0; i < codepoints.len; ++i) {
        REQUIRE((char)codepoints[i] == s[i]);
    }

    end_scratch(scratch);
}

TEST_CASE("MString8 manual cleanup", "[MString8]") {
    i64 allocated_bytes_before = heap_alloc_data.n_active_bytes;
    {
        MString8 s = MSTRING_NT(&heap_alloc);
        s.extend("Hello, World!");
        REQUIRE(s.len == 13);
        REQUIRE(s.allocator == &heap_alloc);
        REQUIRE(heap_alloc_data.n_active_bytes > allocated_bytes_before);
        REQUIRE(heap_alloc_data.n_allocated_bytes > allocated_bytes_before);
        s.destroy();
    }
    REQUIRE(heap_alloc_data.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("MString8 -> String", "[MString8]") {
    i64 allocated_bytes_before = heap_alloc_data.n_active_bytes;
    {
        MString8 m = MSTRING_NT(&heap_alloc);
        m.extend("Hello, World!");
        AString8 s = m;

        REQUIRE(s.len == 13);
        REQUIRE(s.allocator == &heap_alloc);
        REQUIRE(heap_alloc_data.n_active_bytes > allocated_bytes_before);
        REQUIRE(heap_alloc_data.n_allocated_bytes > allocated_bytes_before);
    }
    REQUIRE(heap_alloc_data.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("String -> MString8", "[MString8]") {
    i64 allocated_bytes_before = heap_alloc_data.n_active_bytes;
    {
        AString8 s;
        s.extend("Hello, World!");
        MString8 m = s.release();

        REQUIRE(m.len == 13);
        REQUIRE(m.allocator == &heap_alloc);
        REQUIRE(heap_alloc_data.n_active_bytes > allocated_bytes_before);
        REQUIRE(heap_alloc_data.n_allocated_bytes > allocated_bytes_before);
        m.destroy();
    }
    REQUIRE(heap_alloc_data.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("Seq<String> memory management", "[Seq][String]") {
    i64 allocated_bytes_before = heap_alloc_data.n_active_bytes;
    {
        AArray<AString8> strings;
        REQUIRE(strings.len == 0);

        for(int i = 0; i < 10; ++i) {
            AString8 s = "some string";
            strings.push_back(move(s));
        }

        REQUIRE(strings.len == 10);

        strings[5].extend(" (modified)");
        REQUIRE(strings[5].len > strings[4].len);

        REQUIRE(heap_alloc_data.n_active_bytes > allocated_bytes_before);
    }

    REQUIRE(heap_alloc_data.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("String8 and String operator<", "[String8][String]") {
    // Test String8 operator<
    String8 s1 = S8_LIT("apple");
    String8 s2 = S8_LIT("banana");
    String8 s3 = S8_LIT("app");
    String8 s4 = S8_LIT("apple");
    String8 s5 = S8_LIT("application");

    REQUIRE(s1 < s2);    // "apple" < "banana"
    REQUIRE(!(s2 < s1)); // "banana" not < "apple"
    REQUIRE(s3 < s1);    // "app" < "apple"
    REQUIRE(!(s1 < s3)); // "apple" not < "app"
    REQUIRE(!(s1 < s4)); // "apple" not < "apple"
    REQUIRE(!(s4 < s1)); // "apple" not < "apple"
    REQUIRE(s1 < s5);    // "apple" < "application"
    REQUIRE(!(s5 < s1)); // "application" not < "apple"
    String8 empty1 = S8_LIT("");
    String8 empty2 = S8_LIT("");
    String8 non_empty = S8_LIT("a");
    REQUIRE(!(empty1 < empty2));    // empty not < empty
    REQUIRE(empty1 < non_empty);    // empty < non-empty
    REQUIRE(!(non_empty < empty1)); // non-empty not < empty

    // Test case sensitivity
    String8 lower = S8_LIT("apple");
    String8 upper = S8_LIT("APPLE");

    REQUIRE(upper < lower); // "APPLE" < "apple" (ASCII values)
    REQUIRE(!(lower < upper));
}

TEST_CASE("Utf8Iterator with emoji string", "[Utf8Iterator]") {
    // 👋 is U+1F44B (4 bytes in UTF-8: F0 9F 91 8B)
    // 🌍 is U+1F30D (4 bytes in UTF-8: F0 9F 8C 8D)
    String8 s = S8_LIT("Hi \xF0\x9F\x91\x8B \xF0\x9F\x8C\x8D!");
    ArenaTmp scratch = begin_scratch();

    Array<u32> codepoints = decode_string8(scratch.arena, s);
    REQUIRE(codepoints.len == 7);
    REQUIRE((char)codepoints[0] == 'H');
    REQUIRE((char)codepoints[1] == 'i');
    REQUIRE((char)codepoints[2] == ' ');
    REQUIRE(codepoints[4] == ' ');
    REQUIRE(codepoints[3] == U'👋');
    REQUIRE(codepoints[5] == U'🌍');
    REQUIRE(codepoints[6] == '!');
}
