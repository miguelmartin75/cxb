#define CATCH_CONFIG_MAIN
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>
#include <cxb/cxb-unicode.h>
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
    StringSlice s{.data=const_cast<char*>(test_str), .len=13, .null_term=true};

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
    StringSlice s{.data=nullptr, .len=0, .null_term=true};
    REQUIRE(s.size() == 0);
    REQUIRE(s.empty());
    REQUIRE(s.null_term);
}

TEST_CASE("StringSlice from raw data", "[StringSlice]") {
    char data[] = {'H', 'e', 'l', 'l', 'o'};
    StringSlice s{.data=data, .len=5, .null_term=false};

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
