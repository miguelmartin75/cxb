#define CATCH_CONFIG_MAIN
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb-unicode.h>
#include <cxb/cxb.h>
#include <random>

CXB_INLINE bool string_less_than_forloop(const StringSlice& a, const StringSlice& b) {
    size_t n = a.len < b.len ? a.len : b.len;
    for(size_t i = 0; i < n; ++i) {
        if(a.data[i] < b.data[i]) return true;
        if(a.data[i] > b.data[i]) return false;
    }
    return a.len < b.len;
}

TEST_CASE("String operator< benchmark", "[benchmark][String]") {
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> char_dist('a', 'z');

    AString small_str1;
    AString small_str2;

    for(int i = 0; i < 64; ++i) {
        char c1 = static_cast<char>(char_dist(rng));
        char c2 = static_cast<char>(char_dist(rng));
        small_str1.push_back(c1);
        small_str2.push_back(c2);
    }

    AString medium_str1;
    AString medium_str2;

    for(int i = 0; i < 100000; ++i) {
        char c1 = static_cast<char>(char_dist(rng));
        char c2 = static_cast<char>(char_dist(rng));
        medium_str1.push_back(c1);
        medium_str2.push_back(c2);
    }

    AString large_str1;
    AString large_str2;

    for(int i = 0; i < 1000000; ++i) {
        char c1 = static_cast<char>(char_dist(rng));
        char c2 = static_cast<char>(char_dist(rng));
        large_str1.push_back(c1);
        large_str2.push_back(c2);
    }

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
    AString equal_small = small_str1.copy();
    AString equal_medium = medium_str1.copy();
    AString equal_large = large_str1.copy();

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

TEST_CASE("UTF-8 decoding benchmark - ASCII text", "[.benchmark]") {
    // Create a large ASCII string for benchmarking with varied content
    AString ascii_text;
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

TEST_CASE("UTF-8 validation benchmark", "[.benchmark]") {
    AString ascii_text;
    AString unicode_text;

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
TEST_CASE("UTF-8 decoding benchmark - Mixed Unicode", "[.benchmark]") {
    AString unicode_text;
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
