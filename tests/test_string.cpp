#define CATCH_CONFIG_MAIN
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb-unicode.h>
#include <cxb/cxb.h>
#include <random>

CXB_USE_NS;

TEST_CASE("StringSlice default constructor", "[StringSlice]") {
    StringSlice s = {};
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE_FALSE(s.null_term);
}

TEST_CASE("StringSlice from C string", "[StringSlice]") {
    const char* test_str = "Hello, World!";
    StringSlice s{.data = const_cast<char*>(test_str), .len = 13, .null_term = true};

    REQUIRE(s.size() == 13);
    REQUIRE_FALSE(s.empty());
    REQUIRE(s.null_term);
    REQUIRE(strcmp(s.c_str(), test_str) == 0);

    for(size_t i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == test_str[i]);
    }
}

TEST_CASE("StringSlice from empty C string", "[StringSlice]") {
    StringSlice s = S8_LIT("");
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE(s.null_term);
    REQUIRE(s == S8_LIT(""));
}

TEST_CASE("StringSlice from null pointer", "[StringSlice]") {
    StringSlice s{.data = nullptr, .len = 0, .null_term = true};
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE(s.null_term);
}

TEST_CASE("StringSlice from raw data", "[StringSlice]") {
    char data[] = {'H', 'e', 'l', 'l', 'o'};
    StringSlice s{.data = data, .len = 5, .null_term = false};

    REQUIRE(s.size() == 5);
    REQUIRE(!s.null_term);
    REQUIRE(s.data == data);

    for(size_t i = 0; i < 5; ++i) {
        REQUIRE(s[i] == data[i]);
    }
}

TEST_CASE("String push_back", "[StringSlice]") {
    size_t allocated_bytes = 0;
    {
        String s;
        s.push_back('H');
        s.push_back('i');

        REQUIRE(s.size() == 2);
        REQUIRE(s[0] == 'H');
        REQUIRE(s[1] == 'i');
        REQUIRE(s.null_term);

        allocated_bytes = default_alloc.n_allocated_bytes;
    }
    REQUIRE(default_alloc.n_active_bytes == 0);
    REQUIRE(default_alloc.n_allocated_bytes == allocated_bytes);
}

TEST_CASE("StringSlice push_back with null termination", "[StringSlice]") {
    String s("Hello");
    REQUIRE(s.null_term);
    REQUIRE(s.allocator);
    REQUIRE(s.len == 5);
    REQUIRE(s.size() == 5);

    s.push_back('!');
    REQUIRE(s.size() == 6);
    REQUIRE(s.null_term);

    StringSlice cmp = S8_LIT("Hello!");
    for(int i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == cmp[i]);
    }
    REQUIRE(s == cmp);
    REQUIRE(strcmp(s.c_str(), "Hello!") == 0);
}

TEST_CASE("StringSlice append C string", "[StringSlice]") {
    String s("Hello");
    s.extend(", World!");

    REQUIRE(s.len == 13);
    REQUIRE(s.null_term);
    REQUIRE(s == S8_LIT("Hello, World!"));
    REQUIRE(strcmp(s.c_str(), "Hello, World!") == 0);
}

TEST_CASE("StringSlice append other StringSlice", "[StringSlice]") {
    String s1("Hello");
    StringSlice s2 = S8_LIT(", World!");
    s1.extend(s2);

    REQUIRE(s1.len == 13);
    REQUIRE(s1.null_term);
    REQUIRE(s1 == S8_LIT("Hello, World!"));
}

TEST_CASE("StringSlice append to non-null-terminated", "[StringSlice]") {
    String s;
    s.push_back('H');
    s.push_back('i');
    REQUIRE(s.null_term);

    s.extend(" there");
    REQUIRE(s.size() == 8);
    REQUIRE(s.null_term);

    const char expected[] = "Hi there";
    for(size_t i = 0; i < s.size(); ++i) {
        REQUIRE(s[i] == expected[i]);
    }
    REQUIRE(s == S8_LIT("Hi there"));
}

TEST_CASE("StringSlice resize", "[StringSlice]") {
    String s("Hello");
    s.resize(10, 'X');

    REQUIRE(s.size() == 10);
    REQUIRE(s.null_term);

    REQUIRE(s == S8_LIT("HelloXXXXX"));
}

TEST_CASE("StringSlice resize shrinking", "[StringSlice]") {
    String s("Hello, World!");
    s.resize(5);

    REQUIRE(s.size() == 5);
    REQUIRE(s.null_term);
    REQUIRE(s == S8_LIT("Hello"));
}

TEST_CASE("StringSlice pop_back", "[StringSlice]") {
    String s("Hello");
    char c = s.pop_back();

    REQUIRE(c == 'o');
    REQUIRE(s.size() == 4);
    REQUIRE(s.null_term);
    REQUIRE(s == S8_LIT("Hell"));
}

TEST_CASE("StringSlice slice", "[StringSlice]") {
    StringSlice s = S8_LIT("Hello, World!");
    StringSlice slice1 = s.slice(7);    // "World!"
    StringSlice slice2 = s.slice(0, 5); // "Hello"

    REQUIRE(slice1.size() == 6);
    REQUIRE(slice1.null_term);

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

TEST_CASE("StringSlice copy", "[StringSlice]") {
    String original("Hello, World!");
    String copy = original.copy();

    REQUIRE(copy.size() == original.size());
    REQUIRE(copy.null_term == original.null_term);
    REQUIRE(copy.allocator == original.allocator);
    REQUIRE(copy.data != original.data); // different memory
    REQUIRE(original == copy);
}

TEST_CASE("StringSlice ensure_null_terminated", "[StringSlice]") {
    String s;
    s.push_back('H');
    s.push_back('i');
    REQUIRE(s.null_term);

    s.ensure_null_terminated();
    REQUIRE(s.null_term);
    REQUIRE(s.size() == 2);
    REQUIRE(s == S8_LIT("Hi"));
}

TEST_CASE("Utf8Iterator with ASCII string", "[Utf8Iterator]") {
    StringSlice s = S8_LIT("Hello World");
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
    StringSlice s = S8_LIT("Hi \xF0\x9F\x91\x8B \xF0\x9F\x8C\x8D!");
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

TEST_CASE("UTF-8 decoding benchmark - ASCII text", "[.benchmark]") {
    // Create a large ASCII string for benchmarking with varied content
    String ascii_text;
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> char_dist('A', 'Z');

    // Create varied ASCII content to prevent optimization
    for(int i = 0; i < 1000; ++i) {
        for(int j = 0; j < 45; ++j) {
            ascii_text.push_back(static_cast<char>(char_dist(rng)));
        }
        ascii_text.push_back(' ');
    }

    BENCHMARK("UTF-8 decode ASCII with utf8_decode") {
        size_t pos = 0;
        u64 checksum = 0;

        while(pos < ascii_text.size()) {
            auto result = utf8_decode(ascii_text.data + pos, ascii_text.size() - pos);
            if(!result.valid) break;
            pos += result.bytes_consumed;
            checksum ^= result.codepoint;
        }

        return checksum;
    };

    BENCHMARK("UTF-8 decode ASCII with Utf8Iterator") {
        Utf8Iterator iter(ascii_text);
        u64 checksum = 0;

        while(iter.has_next()) {
            auto result = iter.next();
            if(!result.valid) break;
            checksum ^= result.codepoint; // Prevent optimization
        }

        return checksum;
    };
}

TEST_CASE("UTF-8 decoding benchmark - Mixed Unicode", "[.benchmark]") {
    String unicode_text;
    const char* samples[] = {
        "Hello 🌍 World! ",   // ASCII + emoji
        "Café naïve résumé ", // ASCII + accented chars
        "こんにちは世界 ",    // Japanese
        "🚀🌟💫⭐🎉 ",        // Multiple emojis
        "Привет мир! "        // Cyrillic
    };

    for(int i = 0; i < 200; ++i) {
        for(const char* sample : samples) {
            unicode_text.extend(sample);
        }
    }

    BENCHMARK("UTF-8 decode single") {
        Utf8Iterator iter(unicode_text);
        u64 checksum = 0;

        while(iter.has_next()) {
            auto result = iter.next();
            if(!result.valid) break;
            checksum ^= result.codepoint;
        }

        return checksum;
    };
}

TEST_CASE("UTF-8 validation benchmark", "[.benchmark]") {
    String ascii_text;
    String unicode_text;

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> ascii_dist(32, 126);

    for(int i = 0; i < 10000; ++i) {
        while(ascii_text.size() < 100) {
            ascii_text.push_back(static_cast<char>(ascii_dist(rng)));
        }
        ascii_text.push_back('\n');
    }

    u32 unicode_codepoints[] = {
        0x1F600, 0x1F601, 0x1F602, 0x1F603, 0x1F604, 0x1F605, 0x1F606, 0x1F607, // Smileys
        0x1F680, 0x1F681, 0x1F682, 0x1F683, 0x1F684, 0x1F685, 0x1F686, 0x1F687, // Transportation
        0x1F300, 0x1F301, 0x1F302, 0x1F303, 0x1F304, 0x1F305, 0x1F306, 0x1F307, // Nature
        0x1F30D, 0x1F30E, 0x1F30F, 0x1F311, 0x1F313, 0x1F314, 0x1F315, 0x1F319, // Earth/Moon
        0x1F44D, 0x1F44E, 0x1F44F, 0x1F450, 0x1F451, 0x1F4A9, 0x1F4AA, 0x1F525, // Hands/Objects
        0x2764,  0x2665,  0x2B50,  0x2728,  0x26A1,  0x1F525, 0x1F4A5, 0x1F31F  // Hearts/Stars
    };
    std::uniform_int_distribution<size_t> unicode_dist(0,
                                                       sizeof(unicode_codepoints) / sizeof(unicode_codepoints[0]) - 1);

    for(int i = 0; i < 10000; ++i) {
        while(unicode_text.size() < 100) {
            u32 codepoint = unicode_codepoints[unicode_dist(rng)];
            auto encode_result = utf8_encode(codepoint);
            if(encode_result.valid) {
                for(u8 j = 0; j < encode_result.byte_count; ++j) {
                    unicode_text.extend(S8_DATA(encode_result.bytes, encode_result.byte_count));
                }
            }
        }
        unicode_text.push_back('\n');
    }

    BENCHMARK("Iterate ASCII text") {
        u64 result = 0;
        for(size_t i = 0; i < ascii_text.size(); ++i) {
            result ^= ascii_text[i];
        }
        REQUIRE(result);
        return result;
    };
}

TEST_CASE("MString manual cleanup", "[MString]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        MString s{.data = nullptr, .len = 0, .null_term = true, .allocator = &default_alloc};
        s.extend("Hello, World!");
        REQUIRE(s.len == 13);
        REQUIRE(s.allocator == &default_alloc);
        REQUIRE(default_alloc.n_active_bytes > allocated_bytes_before);
        REQUIRE(default_alloc.n_allocated_bytes > allocated_bytes_before);
        s.destroy();
    }
    REQUIRE(default_alloc.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("MString -> String", "[MString]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        MString m{.data = nullptr, .len = 0, .null_term = true, .allocator = &default_alloc};
        m.extend("Hello, World!");
        String s = m;

        REQUIRE(s.len == 13);
        REQUIRE(s.allocator == &default_alloc);
        REQUIRE(default_alloc.n_active_bytes > allocated_bytes_before);
        REQUIRE(default_alloc.n_allocated_bytes > allocated_bytes_before);
    }
    REQUIRE(default_alloc.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("String -> MString release + leak", "[MString]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        String s;
        s.extend("Hello, World!");
        MString m = s;
        s.release();

        REQUIRE(m.len == 13);
        REQUIRE(m.allocator == &default_alloc);
        REQUIRE(default_alloc.n_active_bytes > allocated_bytes_before);
        REQUIRE(default_alloc.n_allocated_bytes > allocated_bytes_before);
    }
    REQUIRE(default_alloc.n_active_bytes > allocated_bytes_before);
}
TEST_CASE("MString leaks", "[MString]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        MString s{.data = nullptr, .len = 0, .null_term = true, .allocator = &default_alloc};
        s.extend("Hello, World!");
        REQUIRE(s.len == 13);
        REQUIRE(s.allocator == &default_alloc);
        REQUIRE(default_alloc.n_active_bytes > allocated_bytes_before);
        REQUIRE(default_alloc.n_allocated_bytes > allocated_bytes_before);
    }
    REQUIRE(default_alloc.n_active_bytes > allocated_bytes_before);
}

TEST_CASE("Seq<String> memory management", "[Seq][String]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        Seq<String> strings;
        REQUIRE(strings.len == 0);
        REQUIRE(strings.capacity() == CXB_MALLOCATOR_MIN_CAP);

        for(int i = 0; i < 10; ++i) {
            String s;
            s.extend("String #");
            if(i == 0)
                s.extend("0");
            else {
                int temp = i;
                String num_str;
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

        for(int i = 0; i < strings.len; ++i) {
            REQUIRE(strings[i].len > 0);
            REQUIRE(strings[i].null_term);
            REQUIRE(strings[i].allocator);

            StringSlice prefix = strings[i].slice(0, 8);
            REQUIRE(prefix == S8_LIT("String #"));

            StringSlice suffix = strings[i].slice(strings[i].len - 10);
            REQUIRE(suffix == S8_LIT(" - Content"));
        }

        strings[5].extend(" (modified)");
        REQUIRE(strings[5].len > strings[4].len);

        // Add a new string with emoji
        String emoji_str("Hello 🌍!");
        strings.push_back(move(emoji_str));

        REQUIRE(strings.len == 11);
        REQUIRE(strings[10] == S8_LIT("Hello 🌍!"));

        // Add an empty string
        String empty_str;
        strings.push_back(move(empty_str));

        REQUIRE(strings.len == 12);
        REQUIRE(strings[11].len == 0);
        REQUIRE(strings[11].empty());

        // Verify memory is actively being used
        REQUIRE(default_alloc.n_active_bytes > allocated_bytes_before);
    }

    REQUIRE(default_alloc.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("StringSlice free functions (C API)", "[StringSlice][CAPI]") {
    StringSlice s = S8_LIT("Hello");
    REQUIRE(cxb_ss_size(s) == 5);
    REQUIRE_FALSE(cxb_ss_empty(s));
    REQUIRE(cxb_ss_n_bytes(s) == 6); // includes null terminator
    REQUIRE(strcmp(cxb_ss_c_str(s), "Hello") == 0);

    StringSlice slice = cxb_ss_slice(s, 1, 4); // "ell"
    REQUIRE(cxb_ss_size(slice) == 3);
    REQUIRE(!cxb_ss_empty(slice));
    REQUIRE(cxb_ss_c_str(slice) == nullptr); // not null-terminated
}

TEST_CASE("MString free function destroy (C API)", "[MString][CAPI]") {
    size_t mem_before = default_alloc.n_active_bytes;
    {
        MString ms{.data = nullptr, .len = 0, .null_term = true, .allocator = &default_alloc};
        ms.extend("Hello, World!");
        REQUIRE(cxb_mstring_size(ms) == 13);
        cxb_mstring_destroy(&ms);
        REQUIRE(ms.data == nullptr);
    }
    REQUIRE(default_alloc.n_active_bytes == mem_before);
}

TEST_CASE("StringSlice and String operator<", "[StringSlice][String]") {
    // Test StringSlice operator<
    StringSlice s1 = S8_LIT("apple");
    StringSlice s2 = S8_LIT("banana");
    StringSlice s3 = S8_LIT("app");
    StringSlice s4 = S8_LIT("apple");
    StringSlice s5 = S8_LIT("application");

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
    StringSlice empty1 = S8_LIT("");
    StringSlice empty2 = S8_LIT("");
    StringSlice non_empty = S8_LIT("a");

    REQUIRE(!(empty1 < empty2));    // empty not < empty
    REQUIRE(empty1 < non_empty);    // empty < non-empty
    REQUIRE(!(non_empty < empty1)); // non-empty not < empty

    // Test case sensitivity
    StringSlice lower = S8_LIT("apple");
    StringSlice upper = S8_LIT("APPLE");

    REQUIRE(upper < lower); // "APPLE" < "apple" (ASCII values)
    REQUIRE(!(lower < upper));

    // Test with special characters
    StringSlice alpha = S8_LIT("abc");
    StringSlice numeric = S8_LIT("123");
    StringSlice special = S8_LIT("!@#");

    REQUIRE(special < numeric); // "!@#" < "123" (ASCII values)
    REQUIRE(numeric < alpha);   // "123" < "abc" (ASCII values)

    // Test String operator< (should behave the same as StringSlice)
    String str1("apple");
    String str2("banana");
    String str3("app");
    String str4("apple");

    REQUIRE(str1 < str2);    // "apple" < "banana"
    REQUIRE(!(str2 < str1)); // "banana" not < "apple"
    REQUIRE(str3 < str1);    // "app" < "apple"
    REQUIRE(!(str1 < str3)); // "apple" not < "app"
    REQUIRE(!(str1 < str4)); // "apple" not < "apple"
    REQUIRE(!(str4 < str1)); // "apple" not < "apple"

    // Test mixed String and StringSlice comparison
    REQUIRE(str1 < s2); // String("apple") < StringSlice("banana")
    REQUIRE(s3 < str1); // StringSlice("app") < String("apple")

    // Test operator> for StringSlice
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

    // Test mixed String and StringSlice operator> comparison
    REQUIRE(s2 > str1); // StringSlice("banana") > String("apple")
    REQUIRE(str1 > s3); // String("apple") > StringSlice("app")

    // Test that equal strings are not greater than each other
    REQUIRE(!(s1 > s4));         // "apple" not > "apple"
    REQUIRE(!(s4 > s1));         // "apple" not > "apple"
    REQUIRE(!(str1 > str4));     // "apple" not > "apple"
    REQUIRE(!(str4 > str1));     // "apple" not > "apple"
    REQUIRE(!(empty1 > empty2)); // "" not > ""
    REQUIRE(!(empty2 > empty1)); // "" not > ""
}

CXB_INLINE bool string_less_than_forloop(const StringSlice& a, const StringSlice& b) {
    size_t n = a.len < b.len ? a.len : b.len;
    for(size_t i = 0; i < n; ++i) {
        if(a.data[i] < b.data[i]) return true;
        if(a.data[i] > b.data[i]) return false;
    }
    return a.len < b.len;
}

TEST_CASE("String operator< benchmark", "[.benchmark][String]") {
    String small_str1;
    String small_str2;
    for(int i = 0; i < 200; ++i) {
        small_str1.push_back('a' + (i % 26));
        small_str2.push_back('a' + ((i + 1) % 26));
    }

    String medium_str1;
    String medium_str2;
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> char_dist('a', 'z');

    for(int i = 0; i < 100000; ++i) {
        char c1 = static_cast<char>(char_dist(rng));
        char c2 = static_cast<char>(char_dist(rng));
        medium_str1.push_back(c1);
        medium_str2.push_back(c2);
    }

    String large_str1;
    String large_str2;

    for(int i = 0; i < 1000000; ++i) {
        char c1 = static_cast<char>(char_dist(rng));
        char c2 = static_cast<char>(char_dist(rng));
        large_str1.push_back(c1);
        large_str2.push_back(c2);
    }

    // Create pairs with different comparison outcomes
    // Make the second string slightly different to avoid early termination
    small_str2[50] = small_str2[50] + 1;
    medium_str2[50000] = medium_str2[50000] + 1;
    large_str2[500000] = large_str2[500000] + 1;

    // Small string benchmarks
    BENCHMARK("String operator< (memcmp) - Small strings") {
        return small_str1 < small_str2;
    };

    BENCHMARK("String operator< (for loop) - Small strings") {
        return string_less_than_forloop(small_str1, small_str2);
    };

    // Medium string benchmarks
    BENCHMARK("String operator< (memcmp) - Medium strings") {
        return medium_str1 < medium_str2;
    };

    BENCHMARK("String operator< (for loop) - Medium strings") {
        return string_less_than_forloop(medium_str1, medium_str2);
    };

    // Large string benchmarks
    BENCHMARK("String operator< (memcmp) - Large strings") {
        return large_str1 < large_str2;
    };

    BENCHMARK("String operator< (for loop) - Large strings") {
        return string_less_than_forloop(large_str1, large_str2);
    };

    // Additional benchmarks with equal strings (worst case for for loop)
    String equal_small = small_str1.copy();
    String equal_medium = medium_str1.copy();
    String equal_large = large_str1.copy();

    BENCHMARK("String operator< (memcmp) - Equal small strings") {
        return small_str1 < equal_small;
    };

    BENCHMARK("String operator< (for loop) - Equal small strings") {
        return string_less_than_forloop(small_str1, equal_small);
    };

    BENCHMARK("String operator< (memcmp) - Equal medium strings") {
        return medium_str1 < equal_medium;
    };

    BENCHMARK("String operator< (for loop) - Equal medium strings") {
        return string_less_than_forloop(medium_str1, equal_medium);
    };

    BENCHMARK("String operator< (memcmp) - Equal large strings") {
        return large_str1 < equal_large;
    };

    BENCHMARK("String operator< (for loop) - Equal large strings") {
        return string_less_than_forloop(large_str1, equal_large);
    };
}
