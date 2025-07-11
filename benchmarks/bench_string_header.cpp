#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <cxb/cxb.h>

#include <algorithm>
#include <random>
#include <numeric>
#include <vector>
#include <string>

CXB_USE_NS;

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
    for(size_t i = 0; i < len; ++i) s.push_back(random_char(rng));
    return s;
}

// Helper to generate a String2 of given length filled with random chars
String2 make_string2(size_t len, std::mt19937& rng) {
    String2 s;
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

    BENCHMARK("String2 push_back small") {
        String2 s;
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

    BENCHMARK("String2 push_back large") {
        String2 s;
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
    String2 s2_small = make_string2(SMALL_SIZE, rng);

    std::string std_small = make_stdstring(SMALL_SIZE, rng);

    AString a_large = make_astring(LARGE_SIZE, rng);
    String2 s2_large = make_string2(LARGE_SIZE, rng);

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

    BENCHMARK("String2 random access small") {
        volatile size_t sum = 0;
        for(size_t i : idx_small) sum += s2_small[i];
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

    BENCHMARK("String2 random access large") {
        volatile size_t sum = 0;
        for(size_t i : idx_large) sum += s2_large[i];
        return sum;
    };

    BENCHMARK("std::string random access large") {
        volatile size_t sum = 0;
        for(size_t i : idx_large) sum += std_large[i];
        return sum;
    };
}

TEST_CASE("sort benchmark", "[benchmark][String]") {
    std::mt19937 rng(777);

    constexpr size_t N_SMALL = 1000;
    constexpr size_t N_LARGE = 300; // fewer because large strings are expensive

    // Generate base vectors of random strings
    std::vector<AString> base_small_a;
    std::vector<String2> base_small_s2;
    std::vector<std::string> base_small_std;
    base_small_a.reserve(N_SMALL);
    base_small_s2.reserve(N_SMALL);
    base_small_std.reserve(N_SMALL);
    for(size_t i = 0; i < N_SMALL; ++i) {
        size_t len = (i % SMALL_SIZE) + 1; // vary length 1..SMALL_SIZE
        base_small_a.push_back(make_astring(len, rng));
        base_small_s2.push_back(make_string2(len, rng));
        base_small_std.push_back(make_stdstring(len, rng));
    }

    std::vector<AString> base_large_a;
    std::vector<String2> base_large_s2;
    std::vector<std::string> base_large_std;
    base_large_a.reserve(N_LARGE);
    base_large_s2.reserve(N_LARGE);
    base_large_std.reserve(N_LARGE);
    for(size_t i = 0; i < N_LARGE; ++i) {
        size_t len = LARGE_SIZE / 16 + (i % 128); // around 65k chars
        base_large_a.push_back(make_astring(len, rng));
        base_large_s2.push_back(make_string2(len, rng));
        base_large_std.push_back(make_stdstring(len, rng));
    }

    BENCHMARK("AString sort small") {
        std::vector<size_t> idx(base_small_a.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(), [&](size_t lhs, size_t rhs) {
            return base_small_a[lhs] < base_small_a[rhs];
        });
        return idx.front();
    };

    BENCHMARK("String2 sort small") {
        std::vector<size_t> idx(base_small_s2.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(), [&](size_t lhs, size_t rhs) {
            return base_small_s2[lhs] < base_small_s2[rhs];
        });
        return idx.front();
    };

    BENCHMARK("std::string sort small") {
        std::vector<size_t> idx(base_small_std.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(), [&](size_t lhs, size_t rhs) {
            return base_small_std[lhs] < base_small_std[rhs];
        });
        return idx.front();
    };

    BENCHMARK("AString sort large") {
        std::vector<size_t> idx(base_large_a.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(), [&](size_t lhs, size_t rhs) {
            return base_large_a[lhs] < base_large_a[rhs];
        });
        return idx.front();
    };

    BENCHMARK("String2 sort large") {
        std::vector<size_t> idx(base_large_s2.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(), [&](size_t lhs, size_t rhs) {
            return base_large_s2[lhs] < base_large_s2[rhs];
        });
        return idx.front();
    };

    BENCHMARK("std::string sort large") {
        std::vector<size_t> idx(base_large_std.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(), [&](size_t lhs, size_t rhs) {
            return base_large_std[lhs] < base_large_std[rhs];
        });
        return idx.front();
    };
} 