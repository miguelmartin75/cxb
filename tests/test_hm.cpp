#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

size_t hash(const int& x);
#include <cxb/cxb.h>

size_t hash(const int& x) {
    return x; // TODO
}

TEST_CASE("basic", "[HashMap]") {
    Arena* a = get_perm();
    HashMap<int, int> kvs;
    REQUIRE(kvs.put(a, {1, 2}));
    for(auto& kv : kvs) {
        REQUIRE(kv.key == 1);
        REQUIRE(kv.value == 2);
        break;
    }

    REQUIRE(kvs.len == 1);
    auto kv_arr = make_static_array<KvPair<int, int>>({{7, 9}, {3, 5}, {11, 9}});
    REQUIRE(kvs.extend(a, kv_arr));
    REQUIRE(kvs.contains(1));
    REQUIRE(kvs.contains(7));
    REQUIRE(kvs.contains(3));
    REQUIRE(kvs.contains(11));
    REQUIRE(!kvs.contains(2));
    REQUIRE(kvs.len == 4);

    REQUIRE(kvs[1] == 2);
    REQUIRE(kvs.erase(1));
    REQUIRE(kvs.len == 3);
    REQUIRE(!kvs.erase(2));

    REQUIRE(kvs.put(a, {1, 3}));
    REQUIRE(kvs.len == 4);
    REQUIRE(kvs.contains(1));

    REQUIRE(kvs[1] == 3);
};

TEST_CASE("rehash", "[HashMap]") {
    Arena* a = get_perm();
    HashMap<int, int> kvs;
    int i = 0;
    while(true) {
        if(i != 0 && kvs.needs_rehash()) break;
        kvs.put(a, {i, i});
        i += 1;
    }
    REQUIRE(kvs.table.len == CXB_HM_MIN_CAP);
    kvs.put(a, {i, i});

    REQUIRE(kvs.table.len == 2 * CXB_HM_MIN_CAP);
    for(int j = i; j <= i; ++j) {
        REQUIRE(kvs.contains(j));
    }
};

TEST_CASE("MHashMap manual cleanup", "[MHashMap]") {
    i64 allocated_before = heap_alloc_data.n_active_bytes;
    {
        MHashMap<int, int> hm;
        REQUIRE(hm.put({1, 2}));
        REQUIRE(hm.len == 1);
        REQUIRE(heap_alloc_data.n_active_bytes > allocated_before);
        hm.destroy();
    }
    REQUIRE(heap_alloc_data.n_active_bytes == allocated_before);
}

TEST_CASE("AHashMap automatic cleanup", "[AHashMap]") {
    i64 allocated_before = heap_alloc_data.n_active_bytes;
    {
        AHashMap<int, int> hm;
        REQUIRE(hm.put({3, 4}));
        REQUIRE(hm.len == 1);
        REQUIRE(heap_alloc_data.n_active_bytes > allocated_before);
    }
    REQUIRE(heap_alloc_data.n_active_bytes == allocated_before);
}
