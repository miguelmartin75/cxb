#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <unordered_map>
#include <vector>

size_t hash(const int& x);
#include <cxb/cxb.h>

size_t hash(const int& x) {
    return static_cast<size_t>(x);
}

TEST_CASE("AHashMap vs std::unordered_map", "[benchmark][AHashMap]") {
    constexpr int N = 2000;

    std::vector<int> keys;
    keys.reserve(N);
    for(int i = 0; i < N; ++i) keys.push_back(i);

    BENCHMARK("AHashMap<int,int> insert N") {
        AHashMap<int, int> hm;
        hm.reserve(static_cast<size_t>(N * 2));
        for(int i = 0; i < N; ++i) {
            hm.put({keys[i], i});
        }
        return hm.len;
    };

    BENCHMARK("std::unordered_map<int,int> insert N") {
        std::unordered_map<int, int> m;
        m.reserve(N * 2);
        for(int i = 0; i < N; ++i) {
            m.emplace(keys[i], i);
        }
        return m.size();
    };

    AHashMap<int, int> hm_pre;
    hm_pre.reserve(static_cast<size_t>(N * 2));
    for(int i = 0; i < N; ++i) hm_pre.put({keys[i], i});

    std::unordered_map<int, int> m_pre;
    m_pre.reserve(N * 2);
    for(int i = 0; i < N; ++i) m_pre.emplace(keys[i], i);

    BENCHMARK("AHashMap<int,int> lookup N") {
        volatile int sum = 0; // prevent optimization
        for(int i = 0; i < N; ++i) {
            if(hm_pre.contains(keys[i])) sum += hm_pre[keys[i]];
        }
        return sum;
    };

    BENCHMARK("std::unordered_map<int,int> lookup N") {
        volatile int sum = 0; // prevent optimization
        for(int i = 0; i < N; ++i) {
            auto it = m_pre.find(keys[i]);
            if(it != m_pre.end()) sum += it->second;
        }
        return sum;
    };

    BENCHMARK("AHashMap<int,int> erase N") {
        AHashMap<int, int> hm;
        hm.reserve(static_cast<size_t>(N * 2));
        for(int i = 0; i < N; ++i) hm.put({keys[i], i});
        for(int i = 0; i < N; ++i) {
            hm.erase(keys[i]);
        }
        return hm.len;
    };

    BENCHMARK("std::unordered_map<int,int> erase N") {
        std::unordered_map<int, int> m;
        m.reserve(N * 2);
        for(int i = 0; i < N; ++i) m.emplace(keys[i], i);
        for(int i = 0; i < N; ++i) {
            m.erase(keys[i]);
        }
        return m.size();
    };
}
