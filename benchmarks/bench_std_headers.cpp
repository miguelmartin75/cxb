#include <catch2/catch_test_macros.hpp>

// Standard library headers
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <filesystem>
#include <forward_list>
#include <functional>
#include <future>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>
#include <optional>
#include <ostream>
#include <queue>
#include <random>
#include <ratio>
#include <regex>
#include <scoped_allocator>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>

TEST_CASE("stdlib headers usage", "[benchmark]") {
    // vector
    std::vector<int> vec{1, 2, 3};
    REQUIRE(vec.size() == 3);

    // map
    std::map<int, int> m{{1, 10}, {2, 20}};
    REQUIRE(m.at(2) == 20);

    // unordered_map with string key
    std::unordered_map<std::string, int> um{{"a", 1}, {"b", 2}};
    REQUIRE(um["b"] == 2);

    // optional
    std::optional<int> opt = 5;
    REQUIRE(opt.has_value());

    // variant
    std::variant<int, double> var = 3.14;
    REQUIRE(std::holds_alternative<double>(var));

    // array
    std::array<int, 4> arr{{0, 1, 2, 3}};
    REQUIRE(arr[2] == 2);

    // tuple
    auto tup = std::make_tuple(1, 'a', 3.5);
    REQUIRE(std::get<0>(tup) == 1);

    // stack
    std::stack<int> stk;
    stk.push(42);
    REQUIRE(stk.top() == 42);

    // queue
    std::queue<int> q;
    q.push(7);
    REQUIRE(q.front() == 7);
}

TEST_CASE("stdlib low header features", "[benchmark]") {
    // utility – move, swap, pair
    std::pair<int, int> p1{1, 2};
    std::pair<int, int> p2 = std::move(p1);
    std::swap(p1, p2);
    REQUIRE(p2.first == 1);

    // deque
    std::deque<int> dq;
    dq.push_back(42);
    REQUIRE(dq.front() == 42);

    // forward_list
    std::forward_list<int> fl{1, 2, 3};
    REQUIRE(*fl.begin() == 1);

    // list
    std::list<int> li{4, 5};
    li.push_back(6);
    REQUIRE(li.back() == 6);

    // set and unordered_set
    std::set<int> st{3, 1, 2};
    REQUIRE(*st.begin() == 1);
    std::unordered_set<int> uset{7, 8, 9};
    REQUIRE(uset.count(8) == 1);

    // mutex and shared_mutex
    std::mutex mx;
    {
        std::lock_guard<std::mutex> lg(mx);
        REQUIRE(mx.try_lock() == false);
    }
    // shared_mutex usage (C++17)
    std::shared_mutex smx;
    {
        std::shared_lock<std::shared_mutex> sl(smx);
        REQUIRE(smx.try_lock_shared() == false);
    }

    // thread
    int thread_result = 0;
    std::thread th([&thread_result]() { thread_result = 123; });
    th.join();
    REQUIRE(thread_result == 123);

    // exception & system_error
    try {
        throw std::runtime_error("error");
    } catch (const std::exception &e) {
        REQUIRE(std::string(e.what()).find("error") != std::string::npos);
    }
    std::error_code ec; // system_error header
    REQUIRE(!ec);

    // iostream / iomanip / istream / streambuf
    std::stringstream ss;
    ss << std::setw(3) << 7;
    int iv;
    ss >> iv;
    REQUIRE(iv == 7);

    // numeric & ratio & limits
    std::array<int, 4> local_arr{{10, 20, 30, 40}};
    int sum = std::accumulate(local_arr.begin(), local_arr.end(), 0);
    REQUIRE(sum == 100);
    using half_ratio = std::ratio<1, 2>;
    static_assert(half_ratio::num == 1 && half_ratio::den == 2);
    REQUIRE(std::numeric_limits<int>::is_signed);

    // scoped_allocator (compilation only)
    std::scoped_allocator_adaptor<std::allocator<int>> scoped_alloc;
    int* scoped_ptr = scoped_alloc.allocate(1);
    scoped_alloc.deallocate(scoped_ptr, 1);

    // typeindex and typeinfo / type_traits
    std::type_index idx(typeid(int));
    REQUIRE(idx == std::type_index(typeid(int)));
    REQUIRE(std::is_same_v<int, int>);
} 