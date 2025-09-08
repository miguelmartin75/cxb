#include <algorithm>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>
#include <random>
#include <string>
#include <vector>

struct TestInit {
    TestInit() {
        cxb_init(CxbRuntimeParams{.scratch_params = ArenaParams{.reserve_bytes = GB(1), .max_n_blocks = 0},
                                  .perm_params = {}});
    }
} init;

TEST_CASE("merge_sort vs std::sort", "[benchmark][algos]") {
    constexpr int N = 10000;
    std::vector<int> data(N);
    for(int i = 0; i < N; ++i) {
        data[i] = N - i;
    }

    BENCHMARK("merge_sort") {
        std::vector<int> xs = data;
        merge_sort(xs.data(), xs.size());
        return xs[0];
    };

    BENCHMARK("std::sort") {
        std::vector<int> xs = data;
        std::sort(xs.begin(), xs.end());
        return xs[0];
    };
}

TEST_CASE("merge_sort randomized data sweep", "[benchmark][algos]") {
    for(u64 n = 100; n <= 10000000; n *= 10) {
        std::vector<int> data(n);
        std::mt19937 rng(1337);
        std::uniform_int_distribution<int> dist(0, static_cast<int>(n));
        for(auto& x : data) {
            x = dist(rng);
        }

        BENCHMARK_ADVANCED(("merge_sort random N=" + std::to_string(n)).c_str())(Catch::Benchmark::Chronometer meter) {
            meter.measure([&] {
                std::vector<int> xs = data;
                merge_sort(xs.data(), xs.size());
                return xs[0];
            });
        };
        BENCHMARK_ADVANCED(("std::sort random N=" + std::to_string(n)).c_str())(Catch::Benchmark::Chronometer meter) {
            meter.measure([&] {
                std::vector<int> xs = data;
                std::sort(xs.begin(), xs.end());
                return xs[0];
            });
        };
    }
}
