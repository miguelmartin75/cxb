#define CATCH_CONFIG_MAIN
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>
#include <random>

CXB_INLINE bool string_less_than_forloop(const String8& a, const String8& b) {
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

    AString8 small_str1;
    AString8 small_str2;

    for(int i = 0; i < 64; ++i) {
        char c1 = static_cast<char>(char_dist(rng));
        char c2 = static_cast<char>(char_dist(rng));
        small_str1.push_back(c1);
        small_str2.push_back(c2);
    }

    AString8 medium_str1;
    AString8 medium_str2;

    for(int i = 0; i < 100000; ++i) {
        char c1 = static_cast<char>(char_dist(rng));
        char c2 = static_cast<char>(char_dist(rng));
        medium_str1.push_back(c1);
        medium_str2.push_back(c2);
    }

    AString8 large_str1;
    AString8 large_str2;

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
    AString8 equal_small = small_str1.copy();
    AString8 equal_medium = medium_str1.copy();
    AString8 equal_large = large_str1.copy();

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
    AString8 ascii_text;
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> char_dist('A', 'Z');

    // Create varied ASCII content to prevent optimization
    for(int i = 0; i < 1000; ++i) {
        for(int j = 0; j < 45; ++j) {
            ascii_text.push_back(static_cast<char>(char_dist(rng)));
        }
        ascii_text.push_back(' ');
    }

    // BENCHMARK("UTF-8 decode ASCII with utf8_decode") {
    //     size_t pos = 0;
    //     u64 checksum = 0;

    //     while(pos < ascii_text.size()) {
    //         auto result = utf8_decode(ascii_text.data + pos, ascii_text.size() - pos);
    //         if(!result.valid) break;
    //         pos += result.bytes_consumed;
    //         checksum ^= result.codepoint;
    //     }

    //     return checksum;
    // };

    // BENCHMARK("UTF-8 decode ASCII with Utf8Iterator") {
    //     Utf8Iterator iter(ascii_text);
    //     u64 checksum = 0;

    //     while(iter.has_next()) {
    //         auto result = iter.next();
    //         if(!result.valid) break;
    //         checksum ^= result.codepoint; // Prevent optimization
    //     }

    //     return checksum;
    // };
}

TEST_CASE("UTF-8 decoding benchmark - Mixed Unicode", "[.benchmark]") {
    AString8 unicode_text;
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

    // BENCHMARK("UTF-8 decode single") {
    //     Utf8Iterator iter(unicode_text);
    //     u64 checksum = 0;

    //     while(iter.has_next()) {
    //         auto result = iter.next();
    //         if(!result.valid) break;
    //         checksum ^= result.codepoint;
    //     }

    //     return checksum;
    // };
}
