#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>

struct Pair {
    int x;
    int y;
};

TEST_CASE("simple initializer list", "AArray") {
    Array<int> xs = {get_perm(), {1, 2, 3}};
    REQUIRE(xs.len == 3);
    REQUIRE(xs[0] == 1);
    REQUIRE(xs[1] == 2);
    REQUIRE(xs[2] == 3);
    xs.push_back(get_perm(), 5);
    REQUIRE(xs.len == 4);
    REQUIRE(xs[3] == 5);
}

TEST_CASE("push_back", "AArray") {
    i64 allocated_bytes = 0;
    {
        AArray<int> xs;
        REQUIRE(xs.len == 0);

        for(int i = 0; i < 256; ++i) {
            xs.push_back(i);
        }
        i64 active_bytes = heap_alloc_data.n_active_bytes;
        allocated_bytes = heap_alloc_data.n_allocated_bytes;
        REQUIRE(active_bytes == allocated_bytes);

        REQUIRE(xs.len == 256);
        for(int i = 0; i < 256; ++i) {
            REQUIRE(xs[i] == i);
        }
    }
    REQUIRE(heap_alloc_data.n_active_bytes == 0);
    REQUIRE(heap_alloc_data.n_allocated_bytes == allocated_bytes);
}

TEST_CASE("copy", "AArray") {
    i64 allocated_bytes_before = heap_alloc_data.n_active_bytes;
    {
        AArray<int> xs;
        xs.resize(64, 2);
        for(u64 i = 0; i < xs.len; ++i) {
            REQUIRE(xs[i] == 2);
        }

        AArray<int> copy = xs.copy();
        REQUIRE(copy.allocator == xs.allocator);
        REQUIRE(copy.data != xs.data);
    }
    REQUIRE(heap_alloc_data.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("nested", "AArray") {
    i64 allocated_bytes_before = heap_alloc_data.n_active_bytes;
    {
        AArray<AArray<int>> nested;
        REQUIRE(nested.len == 0);

        for(int i = 0; i < 10; ++i) {
            AArray<int> inner;
            for(int j = 0; j < i + 1; ++j) {
                inner.push_back(i * 10 + j);
            }
            nested.push_back(::move(inner));
        }

        REQUIRE(nested.len == 10);

        for(u64 i = 0; i < nested.len; ++i) {
            REQUIRE(nested[i].len == i + 1);
            for(u64 j = 0; j < nested[i].len; ++j) {
                REQUIRE(nested[i][j] == static_cast<int>(i * 10 + j));
            }
        }

        nested[5].push_back(999);
        REQUIRE(nested[5].len == 7);
        REQUIRE(nested[5][6] == 999);

        AArray<int> new_inner;
        for(int k = 0; k < 5; ++k) {
            new_inner.push_back(k * 100);
        }
        nested.push_back(move(new_inner));

        REQUIRE(nested.len == 11);
        REQUIRE(nested[10].len == 5);
        REQUIRE(nested[10][0] == 0);
        REQUIRE(nested[10][4] == 400);

        REQUIRE(heap_alloc_data.n_active_bytes > allocated_bytes_before);
    }

    REQUIRE(heap_alloc_data.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("operator<", "AArray") {
    i64 allocated_bytes_before = heap_alloc_data.n_active_bytes;
    {
        AArray<int> seq1 = {1, 2, 3};
        AArray<int> seq2 = {1, 2, 3, 4};

        REQUIRE(seq1 < seq2); // shorter sequence is less than longer with same prefix
        REQUIRE(!(seq2 < seq1));

        AArray<int> seq3 = {1, 2, 2};
        REQUIRE(seq3 < seq1); // [1,2,2] < [1,2,3]
        REQUIRE(!(seq1 < seq3));

        // Test with different first element
        AArray<int> seq4 = {0, 5, 10};
        REQUIRE(seq4 < seq1); // [0,5,10] < [1,2,3]
        REQUIRE(!(seq1 < seq4));

        // Test lexicographic order with strings
        AArray<AString8> str_seq1;
        AArray<AString8> str_seq2;

        str_seq1.push_back(AString8("apple"));
        str_seq1.push_back(AString8("banana"));

        str_seq2.push_back(AString8("apple"));
        str_seq2.push_back(AString8("cherry"));

        REQUIRE(str_seq1 < str_seq2); // "banana" < "cherry"
        REQUIRE(!(str_seq2 < str_seq1));

        // Test identical sequences
        AArray<int> seq5 = {1, 2, 3};
        REQUIRE(seq1 == seq5);
        REQUIRE(seq5 == seq1);
    }
    REQUIRE(heap_alloc_data.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("array_emplace_back and pop_back", "Array") {
    AArenaTmp tmp = begin_scratch();
    Arena* a = tmp.arena;
    Array<Pair> xs{arena_push_fast<Pair>(a, 0), 0};
    array_emplace_back(xs, a, 1, 2);
    array_emplace_back(xs, a, 3, 4);
    REQUIRE(xs.len == 2);
    REQUIRE(xs[0].x == 1);
    REQUIRE(xs[0].y == 2);
    REQUIRE(xs[1].x == 3);
    REQUIRE(xs[1].y == 4);
    array_pop_back(xs, a);
    REQUIRE(xs.len == 1);
    REQUIRE(xs[0].x == 1);
    REQUIRE(xs[0].y == 2);
}

TEST_CASE("array_insert and resize", "Array") {
    AArenaTmp tmp = begin_scratch();
    Arena* a = tmp.arena;
    Array<int> xs = {a, {1, 2, 3}};
    int raw[] = {4, 5};
    Array<int> to_insert{raw, 2};
    array_insert(xs, a, to_insert, 1);
    REQUIRE(xs.len == 5);
    REQUIRE(xs[0] == 1);
    REQUIRE(xs[1] == 4);
    REQUIRE(xs[2] == 5);
    REQUIRE(xs[3] == 2);
    REQUIRE(xs[4] == 3);
    array_resize(xs, a, 7, 9);
    REQUIRE(xs.len == 7);
    REQUIRE(xs[5] == 9);
    REQUIRE(xs[6] == 9);
    array_resize(xs, a, 3);
    REQUIRE(xs.len == 3);
    REQUIRE(xs[0] == 1);
    REQUIRE(xs[1] == 4);
    REQUIRE(xs[2] == 5);
}
