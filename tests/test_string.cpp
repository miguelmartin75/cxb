#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb-unicode.h>
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
    String8 s = S8_LIT("");
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
        AString s;
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
    AString s("Hello");
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
    AString s("Hello");
    s.extend(", World!");

    REQUIRE(s.len == 13);
    REQUIRE_FALSE(s.not_null_term);
    REQUIRE(s == S8_LIT("Hello, World!"));
    REQUIRE(strcmp(s.c_str(), "Hello, World!") == 0);
}

TEST_CASE("String8 append other String8", "[String8]") {
    AString s1("Hello");
    String8 s2 = S8_LIT(", World!");
    s1.extend(s2);

    REQUIRE(s1.len == 13);
    REQUIRE_FALSE(s1.not_null_term);
    REQUIRE(s1 == S8_LIT("Hello, World!"));
}

TEST_CASE("String8 append to non-null-terminated", "[String8]") {
    AString s;
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
    AString s("Hello");
    s.resize(10, 'X');

    REQUIRE(s.size() == 10);
    REQUIRE_FALSE(s.not_null_term);

    REQUIRE(s == S8_LIT("HelloXXXXX"));
}

TEST_CASE("String8 resize shrinking", "[String8]") {
    AString s("Hello, World!");
    s.resize(5);

    REQUIRE(s.size() == 5);
    REQUIRE_FALSE(s.not_null_term);
    REQUIRE(s == S8_LIT("Hello"));
}

TEST_CASE("String8 pop_back", "[String8]") {
    AString s("Hello");
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
    AString original("Hello, World!");
    AString copy = original.copy();

    REQUIRE(copy.size() == original.size());
    REQUIRE(copy.not_null_term == original.not_null_term);
    REQUIRE(copy.allocator == original.allocator);
    REQUIRE(copy.data != original.data); // different memory
    REQUIRE(original == copy);
}

TEST_CASE("String8 ensure_not_null_terminated", "[String8]") {
    AString s = {};
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
    Utf8Iterator iter(s);

    // Check that we can iterate through all ASCII characters
    REQUIRE(iter.has_next());

    // Test first character
    auto result = iter.next();
    REQUIRE(result.valid);
    REQUIRE(result.codepoint == 'H');
    REQUIRE(result.bytes_consumed == 1);

    // Test space character
    iter.pos = 5; // Move to space
    result = iter.next();
    REQUIRE(result.valid);
    REQUIRE(result.codepoint == ' ');
    REQUIRE(result.bytes_consumed == 1);

    // Test last character
    iter.pos = 10; // Move to 'd'
    result = iter.next();
    REQUIRE(result.valid);
    REQUIRE(result.codepoint == 'd');
    REQUIRE(result.bytes_consumed == 1);
    REQUIRE_FALSE(iter.has_next());

    // Test peek functionality
    iter.reset();
    auto peek_result = iter.peek();
    REQUIRE(peek_result == 'H');
    REQUIRE(iter.pos == 0); // Position shouldn't change after peek

    // Verify next() gives same result as peek
    auto next_result = iter.next();
    REQUIRE(next_result.codepoint == peek_result);
    REQUIRE(iter.pos == 1); // Position should advance after next()
}

TEST_CASE("Utf8Iterator with emoji string", "[Utf8Iterator]") {
    // String with mixed ASCII and emojis: "Hi 👋 🌍!"
    // 👋 is U+1F44B (4 bytes in UTF-8: F0 9F 91 8B)
    // 🌍 is U+1F30D (4 bytes in UTF-8: F0 9F 8C 8D)
    String8 s = S8_LIT("Hi \xF0\x9F\x91\x8B \xF0\x9F\x8C\x8D!");
    Utf8Iterator iter(s);

    // Test ASCII 'H'
    REQUIRE(iter.has_next());
    auto result = iter.next();
    REQUIRE(result.valid);
    REQUIRE(result.codepoint == 'H');
    REQUIRE(result.bytes_consumed == 1);

    // Test ASCII 'i'
    result = iter.next();
    REQUIRE(result.valid);
    REQUIRE(result.codepoint == 'i');
    REQUIRE(result.bytes_consumed == 1);

    // Test ASCII space
    result = iter.next();
    REQUIRE(result.valid);
    REQUIRE(result.codepoint == ' ');
    REQUIRE(result.bytes_consumed == 1);

    // Test waving hand emoji 👋 (U+1F44B)
    result = iter.next();
    REQUIRE(result.valid);
    REQUIRE(result.codepoint == 0x1F44B);
    REQUIRE(result.bytes_consumed == 4);

    // Test ASCII space
    result = iter.next();
    REQUIRE(result.valid);
    REQUIRE(result.codepoint == ' ');
    REQUIRE(result.bytes_consumed == 1);

    // Test earth emoji 🌍 (U+1F30D)
    result = iter.next();
    REQUIRE(result.valid);
    REQUIRE(result.codepoint == 0x1F30D);
    REQUIRE(result.bytes_consumed == 4);

    // Test ASCII '!'
    result = iter.next();
    REQUIRE(result.valid);
    REQUIRE(result.codepoint == '!');
    REQUIRE(result.bytes_consumed == 1);

    // Should be at end
    REQUIRE_FALSE(iter.has_next());

    // Test reset and peek with emoji
    iter.reset();
    iter.pos = 3; // Position at start of first emoji
    auto peek_result = iter.peek();
    REQUIRE(peek_result == 0x1F44B);
    REQUIRE(iter.pos == 3); // Position unchanged after peek

    // Verify next gives same result
    auto next_result = iter.next();
    REQUIRE(next_result.codepoint == peek_result);
    REQUIRE(iter.pos == 7); // Position advanced by 4 bytes
}

TEST_CASE("MString manual cleanup", "[MString]") {
    i64 allocated_bytes_before = heap_alloc_data.n_active_bytes;
    {
        MString s = MSTRING_NT(&heap_alloc);
        s.extend("Hello, World!");
        REQUIRE(s.len == 13);
        REQUIRE(s.allocator == &heap_alloc);
        REQUIRE(heap_alloc_data.n_active_bytes > allocated_bytes_before);
        REQUIRE(heap_alloc_data.n_allocated_bytes > allocated_bytes_before);
        s.destroy();
    }
    REQUIRE(heap_alloc_data.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("MString -> String", "[MString]") {
    i64 allocated_bytes_before = heap_alloc_data.n_active_bytes;
    {
        MString m = MSTRING_NT(&heap_alloc);
        m.extend("Hello, World!");
        AString s = m;

        REQUIRE(s.len == 13);
        REQUIRE(s.allocator == &heap_alloc);
        REQUIRE(heap_alloc_data.n_active_bytes > allocated_bytes_before);
        REQUIRE(heap_alloc_data.n_allocated_bytes > allocated_bytes_before);
    }
    REQUIRE(heap_alloc_data.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("String -> MString", "[MString]") {
    i64 allocated_bytes_before = heap_alloc_data.n_active_bytes;
    {
        AString s;
        s.extend("Hello, World!");
        MString m = s.release();

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
        AArray<AString> strings;
        REQUIRE(strings.len == 0);
        // REQUIRE(strings.capacity() == CXB_MALLOCATOR_MIN_CAP);

        for(int i = 0; i < 10; ++i) {
            AString s;
            s.extend("String #");
            if(i == 0)
                s.extend("0");
            else {
                int temp = i;
                AString num_str;
                while(temp > 0) {
                    num_str.push_back('0' + (temp % 10));
                    temp /= 10;
                }
                // Reverse the digits
                for(int j = num_str.len - 1; j >= 0; --j) {
                    s.push_back(num_str[j]);
                }
            }
            s.extend(" - Content");
            strings.push_back(move(s));
        }

        REQUIRE(strings.len == 10);

        for(u64 i = 0; i < strings.len; ++i) {
            REQUIRE(strings[i].len > 0);
            REQUIRE_FALSE(strings[i].not_null_term);
            REQUIRE(strings[i].allocator);

            String8 prefix = strings[i].slice(0, 7);
            REQUIRE(prefix == S8_LIT("String #"));

            String8 suffix = strings[i].slice(-10);
            REQUIRE(suffix == S8_LIT(" - Content"));
        }

        strings[5].extend(" (modified)");
        REQUIRE(strings[5].len > strings[4].len);

        // Add a new string with emoji
        AString emoji_str("Hello 🌍!");
        strings.push_back(move(emoji_str));

        REQUIRE(strings.len == 11);
        REQUIRE(strings[10] == S8_LIT("Hello 🌍!"));

        // Add an empty string
        AString empty_str;
        strings.push_back(move(empty_str));

        REQUIRE(strings.len == 12);
        REQUIRE(strings[11].len == 0);
        REQUIRE(strings[11].empty());

        // Verify memory is actively being used
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

    // Basic lexicographic comparison
    REQUIRE(s1 < s2);    // "apple" < "banana"
    REQUIRE(!(s2 < s1)); // "banana" not < "apple"

    // Prefix comparison
    REQUIRE(s3 < s1);    // "app" < "apple"
    REQUIRE(!(s1 < s3)); // "apple" not < "app"

    // Equal strings
    REQUIRE(!(s1 < s4)); // "apple" not < "apple"
    REQUIRE(!(s4 < s1)); // "apple" not < "apple"

    // Longer vs shorter with same prefix
    REQUIRE(s1 < s5);    // "apple" < "application"
    REQUIRE(!(s5 < s1)); // "application" not < "apple"

    // Test with empty strings
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

    // Test with special characters
    String8 alpha = S8_LIT("abc");
    String8 numeric = S8_LIT("123");
    String8 special = S8_LIT("!@#");

    REQUIRE(special < numeric); // "!@#" < "123" (ASCII values)
    REQUIRE(numeric < alpha);   // "123" < "abc" (ASCII values)

    // Test String operator< (should behave the same as String8)
    AString str1("apple");
    AString str2("banana");
    AString str3("app");
    AString str4("apple");

    REQUIRE(str1 < str2);    // "apple" < "banana"
    REQUIRE(!(str2 < str1)); // "banana" not < "apple"
    REQUIRE(str3 < str1);    // "app" < "apple"
    REQUIRE(!(str1 < str3)); // "apple" not < "app"
    REQUIRE(!(str1 < str4)); // "apple" not < "apple"
    REQUIRE(!(str4 < str1)); // "apple" not < "apple"

    // Test mixed String and String8 comparison
    REQUIRE(str1 < s2); // String("apple") < String8("banana")
    REQUIRE(s3 < str1); // String8("app") < String("apple")

    // Test operator> for String8
    REQUIRE(s2 > s1);               // "banana" > "apple"
    REQUIRE(!(s1 > s2));            // "apple" not > "banana"
    REQUIRE(s1 > s3);               // "apple" > "app"
    REQUIRE(!(s3 > s1));            // "app" not > "apple"
    REQUIRE(s5 > s1);               // "application" > "apple"
    REQUIRE(!(s1 > s5));            // "apple" not > "application"
    REQUIRE(non_empty > empty1);    // "a" > ""
    REQUIRE(!(empty1 > non_empty)); // "" not > "a"
    REQUIRE(lower > upper);         // "apple" > "APPLE" (ASCII values)
    REQUIRE(!(upper > lower));      // "APPLE" not > "apple"
    REQUIRE(alpha > numeric);       // "abc" > "123" (ASCII values)
    REQUIRE(numeric > special);     // "123" > "!@#" (ASCII values)

    // Test operator> for String
    REQUIRE(str2 > str1);    // "banana" > "apple"
    REQUIRE(!(str1 > str2)); // "apple" not > "banana"
    REQUIRE(str1 > str3);    // "apple" > "app"
    REQUIRE(!(str3 > str1)); // "app" not > "apple"

    // Test mixed String and String8 operator> comparison
    REQUIRE(s2 > str1); // String8("banana") > String("apple")
    REQUIRE(str1 > s3); // String("apple") > String8("app")

    // Test that equal strings are not greater than each other
    REQUIRE(!(s1 > s4));         // "apple" not > "apple"
    REQUIRE(!(s4 > s1));         // "apple" not > "apple"
    REQUIRE(!(str1 > str4));     // "apple" not > "apple"
    REQUIRE(!(str4 > str1));     // "apple" not > "apple"
    REQUIRE(!(empty1 > empty2)); // "" not > ""
    REQUIRE(!(empty2 > empty1)); // "" not > ""
}
