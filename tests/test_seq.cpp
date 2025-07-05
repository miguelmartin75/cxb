#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>
CXB_USE_NS;

TEST_CASE("push_back", "[Seq]") {
    size_t allocated_bytes = 0;
    {
        Seq<int> xs;
        REQUIRE(xs.len == 0);
        REQUIRE(xs.capacity() == CXB_MALLOCATOR_MIN_CAP);

        for(int i = 0; i < 256; ++i) {
            xs.push_back(i);
        }
        size_t active_bytes = default_alloc.n_active_bytes;
        allocated_bytes = default_alloc.n_allocated_bytes;
        REQUIRE(active_bytes == allocated_bytes);

        REQUIRE(xs.len == 256);
        for(int i = 0; i < 256; ++i) {
            REQUIRE(xs[i] == i);
        }
    }
    REQUIRE(default_alloc.n_active_bytes == 0);
    REQUIRE(default_alloc.n_allocated_bytes == allocated_bytes);
}

TEST_CASE("copy", "[Seq]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        Seq<int> xs;
        xs.resize(64, 2);
        for(int i = 0; i < xs.len; ++i) {
            REQUIRE(xs[i] == 2);
        }

        Seq<int> view = xs.slice();
        REQUIRE(view.allocator == nullptr);

        Seq<int> real_copy = xs.copy();
        REQUIRE(real_copy.allocator == xs.allocator);
        REQUIRE(real_copy.data != xs.data);
    }
    REQUIRE(default_alloc.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("nested_seq", "[Seq]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        Seq<Seq<int>> nested;
        REQUIRE(nested.len == 0);
        REQUIRE(nested.capacity() == CXB_MALLOCATOR_MIN_CAP);

        for(int i = 0; i < 10; ++i) {
            Seq<int> inner;
            for(int j = 0; j < i + 1; ++j) {
                inner.push_back(i * 10 + j);
            }
            nested.push_back(::move(inner));
        }

        REQUIRE(nested.len == 10);

        for(int i = 0; i < nested.len; ++i) {
            REQUIRE(nested[i].len == i + 1);
            for(int j = 0; j < nested[i].len; ++j) {
                REQUIRE(nested[i][j] == i * 10 + j);
            }
        }

        nested[5].push_back(999);
        REQUIRE(nested[5].len == 7);
        REQUIRE(nested[5][6] == 999);

        Seq<int> new_inner;
        for(int k = 0; k < 5; ++k) {
            new_inner.push_back(k * 100);
        }
        nested.push_back(move(new_inner));

        REQUIRE(nested.len == 11);
        REQUIRE(nested[10].len == 5);
        REQUIRE(nested[10][0] == 0);
        REQUIRE(nested[10][4] == 400);

        REQUIRE(default_alloc.n_active_bytes > allocated_bytes_before);
    }

    REQUIRE(default_alloc.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("Seq operator<", "[Seq]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        Seq<int> seq1;
        Seq<int> seq2;

        // Empty sequences should be equal
        REQUIRE(seq1 == seq2);
        REQUIRE(seq2 == seq1);

        // Test with different lengths
        seq1.push_back(1);
        seq1.push_back(2);
        seq1.push_back(3);

        seq2.push_back(1);
        seq2.push_back(2);
        seq2.push_back(3);
        seq2.push_back(4);

        REQUIRE(seq1 < seq2); // shorter sequence is less than longer with same prefix
        REQUIRE(!(seq2 < seq1));

        // Test with different values
        Seq<int> seq3;
        seq3.push_back(1);
        seq3.push_back(2);
        seq3.push_back(2); // smaller value at same position

        REQUIRE(seq3 < seq1); // [1,2,2] < [1,2,3]
        REQUIRE(!(seq1 < seq3));

        // Test with different first element
        Seq<int> seq4;
        seq4.push_back(0);
        seq4.push_back(5);
        seq4.push_back(10);

        REQUIRE(seq4 < seq1); // [0,5,10] < [1,2,3]
        REQUIRE(!(seq1 < seq4));

        // Test lexicographic order with strings
        Seq<String> str_seq1;
        Seq<String> str_seq2;

        str_seq1.push_back(String("apple"));
        str_seq1.push_back(String("banana"));

        str_seq2.push_back(String("apple"));
        str_seq2.push_back(String("cherry"));

        REQUIRE(str_seq1 < str_seq2); // "banana" < "cherry"
        REQUIRE(!(str_seq2 < str_seq1));

        // Test identical sequences
        Seq<int> seq5;
        seq5.push_back(1);
        seq5.push_back(2);
        seq5.push_back(3);

        REQUIRE(seq1 == seq5);
        REQUIRE(seq5 == seq1);
    }
    REQUIRE(default_alloc.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("Seq operator==", "[Seq]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        Seq<int> seq1;
        Seq<int> seq2;

        // Empty sequences should be equal
        REQUIRE(seq1 == seq2);
        REQUIRE(seq2 == seq1);

        // Add same elements to both sequences
        seq1.push_back(1);
        seq1.push_back(2);
        seq1.push_back(3);

        seq2.push_back(1);
        seq2.push_back(2);
        seq2.push_back(3);

        REQUIRE(seq1 == seq2);
        REQUIRE(seq2 == seq1);

        // Test different lengths
        seq2.push_back(4);
        REQUIRE(!(seq1 == seq2));
        REQUIRE(!(seq2 == seq1));

        // Test different values
        Seq<int> seq3;
        seq3.push_back(1);
        seq3.push_back(2);
        seq3.push_back(2); // different value

        REQUIRE(!(seq1 == seq3));
        REQUIRE(!(seq3 == seq1));

        // Test with strings
        Seq<String> str_seq1;
        Seq<String> str_seq2;

        str_seq1.push_back(String("hello"));
        str_seq1.push_back(String("world"));

        str_seq2.push_back(String("hello"));
        str_seq2.push_back(String("world"));

        REQUIRE(str_seq1 == str_seq2);
        REQUIRE(str_seq2 == str_seq1);

        // Different strings
        str_seq2.pop_back();
        str_seq2.push_back(String("test"));

        REQUIRE(!(str_seq1 == str_seq2));
        REQUIRE(!(str_seq2 == str_seq1));

        // Test self-equality
        REQUIRE(seq1 == seq1);
        REQUIRE(str_seq1 == str_seq1);
    }
    REQUIRE(default_alloc.n_active_bytes == allocated_bytes_before);
}

TEST_CASE("Seq operator> and operator!=", "[Seq]") {
    size_t allocated_bytes_before = default_alloc.n_active_bytes;
    {
        Seq<int> seq1;
        Seq<int> seq2;

        // Empty sequences should be equal (not greater than each other)
        REQUIRE(!(seq1 > seq2));
        REQUIRE(!(seq2 > seq1));
        REQUIRE(!(seq1 != seq2)); // should be equal

        // Test with different lengths
        seq1.push_back(1);
        seq1.push_back(2);
        seq1.push_back(3);

        seq2.push_back(1);
        seq2.push_back(2);
        seq2.push_back(3);
        seq2.push_back(4);

        REQUIRE(seq2 > seq1); // longer sequence is greater than shorter with same prefix
        REQUIRE(!(seq1 > seq2));
        REQUIRE(seq1 != seq2); // different lengths should not be equal
        REQUIRE(seq2 != seq1);

        // Test with different values
        Seq<int> seq3;
        seq3.push_back(1);
        seq3.push_back(2);
        seq3.push_back(4); // larger value at same position

        REQUIRE(seq3 > seq1); // [1,2,4] > [1,2,3]
        REQUIRE(!(seq1 > seq3));
        REQUIRE(seq3 != seq1);
        REQUIRE(seq1 != seq3);

        // Test with different first element
        Seq<int> seq4;
        seq4.push_back(2);
        seq4.push_back(1);
        seq4.push_back(1);

        REQUIRE(seq4 > seq1); // [2,1,1] > [1,2,3]
        REQUIRE(!(seq1 > seq4));
        REQUIRE(seq4 != seq1);
        REQUIRE(seq1 != seq4);

        // Test with strings
        Seq<String> str_seq1;
        Seq<String> str_seq2;

        str_seq1.push_back(String("apple"));
        str_seq1.push_back(String("banana"));

        str_seq2.push_back(String("apple"));
        str_seq2.push_back(String("cherry"));

        REQUIRE(str_seq2 > str_seq1); // "cherry" > "banana"
        REQUIRE(!(str_seq1 > str_seq2));
        REQUIRE(str_seq1 != str_seq2);
        REQUIRE(str_seq2 != str_seq1);

        // Test identical sequences
        Seq<int> seq5;
        seq5.push_back(1);
        seq5.push_back(2);
        seq5.push_back(3);

        REQUIRE(!(seq1 > seq5));
        REQUIRE(!(seq5 > seq1));
        REQUIRE(!(seq1 != seq5)); // should be equal
        REQUIRE(!(seq5 != seq1));
    }
    REQUIRE(default_alloc.n_active_bytes == allocated_bytes_before);
}
