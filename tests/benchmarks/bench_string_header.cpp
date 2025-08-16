#include <algorithm>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace {
constexpr size_t SMALL_SIZE = 1024;
constexpr size_t LARGE_SIZE = 1 << 24; // 16 MiB

char random_char(std::mt19937& rng) {
    static std::uniform_int_distribution<int> dist('a', 'z');
    return static_cast<char>(dist(rng));
}

// Helper to generate an AString of given length filled with random chars
AString make_astring(size_t len, std::mt19937& rng) {
    AString s;
    s.reserve(len);
    for(size_t i = 0; i < len; ++i) s.push_back(random_char(rng));
    return s;
}

// Helper to generate std::string of given length filled with random chars
std::string make_stdstring(size_t len, std::mt19937& rng) {
    std::string s;
    s.reserve(len);
    for(size_t i = 0; i < len; ++i) s.push_back(random_char(rng));
    return s;
}
} // namespace

TEST_CASE("push_back benchmark", "[benchmark][String]") {
    BENCHMARK("AString push_back small") {
        AString s;
        s.reserve(SMALL_SIZE);
        for(size_t i = 0; i < SMALL_SIZE; ++i) {
            s.push_back('x');
        }
        return s.size();
    };

    BENCHMARK("std::string push_back small") {
        std::string s;
        s.reserve(SMALL_SIZE);
        for(size_t i = 0; i < SMALL_SIZE; ++i) {
            s.push_back('x');
        }
        return s.size();
    };

    BENCHMARK("AString push_back large") {
        AString s;
        s.reserve(LARGE_SIZE);
        for(size_t i = 0; i < LARGE_SIZE; ++i) {
            s.push_back('x');
        }
        return s.size();
    };

    BENCHMARK("std::string push_back large") {
        std::string s;
        s.reserve(LARGE_SIZE);
        for(size_t i = 0; i < LARGE_SIZE; ++i) {
            s.push_back('x');
        }
        return s.size();
    };
}

TEST_CASE("random access benchmark", "[benchmark][String]") {
    std::mt19937 rng(12345);

    // Prepare test strings
    AString a_small = make_astring(SMALL_SIZE, rng);

    std::string std_small = make_stdstring(SMALL_SIZE, rng);

    AString a_large = make_astring(LARGE_SIZE, rng);

    std::string std_large = make_stdstring(LARGE_SIZE, rng);

    // Precompute random indices
    constexpr size_t N_INDICES = 2048;
    std::vector<size_t> idx_small(N_INDICES), idx_large(N_INDICES);
    std::uniform_int_distribution<size_t> dist_small(0, SMALL_SIZE - 1);
    std::uniform_int_distribution<size_t> dist_large(0, LARGE_SIZE - 1);
    for(size_t& v : idx_small) v = dist_small(rng);
    for(size_t& v : idx_large) v = dist_large(rng);

    BENCHMARK("AString random access small") {
        volatile size_t sum = 0;
        for(size_t i : idx_small) sum += a_small[i];
        return sum;
    };

    BENCHMARK("std::string random access small") {
        volatile size_t sum = 0;
        for(size_t i : idx_small) sum += std_small[i];
        return sum;
    };

    BENCHMARK("AString random access large") {
        volatile size_t sum = 0;
        for(size_t i : idx_large) sum += a_large[i];
        return sum;
    };

    BENCHMARK("std::string random access large") {
        volatile size_t sum = 0;
        for(size_t i : idx_large) sum += std_large[i];
        return sum;
    };
}
