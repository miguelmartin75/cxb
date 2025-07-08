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
    std::vector<std::pair<int, std::string>> vec{{1, "one"}, {2, "two"}, {3, "three"}};
    REQUIRE(vec.size() == 3);

    // map
    std::map<std::string, std::pair<std::string, std::string>> m{{"a", {"x", "y"}}, {"b", {"y", "z"}}};
    REQUIRE(m.at("b").second == "z");

    // unordered_map with string key
    std::unordered_map<std::string, std::vector<int>> um{{"a", {1, 2}}, {"b", {3, 4}}};
    REQUIRE(um["b"][1] == 4);

    // optional
    std::optional<std::pair<int, std::string>> opt = std::make_pair(5, "five");
    REQUIRE(opt.has_value());

    // variant
    std::variant<int, double, std::string> var = std::string("hello");
    REQUIRE(std::holds_alternative<std::string>(var));

    // array
    std::array<std::pair<int, std::string>, 4> arr{{{0, "zero"}, {1, "one"}, {2, "two"}, {3, "three"}}};
    REQUIRE(arr[2].second == "two");

    // tuple
    auto tup = std::make_tuple(std::string("id"), std::vector<int>{1, 2, 3}, 3.5);
    REQUIRE(std::get<1>(tup).size() == 3);

    // stack
    std::stack<std::vector<int>> stk;
    stk.push({1, 2});
    REQUIRE(stk.top().at(1) == 2);

    // queue
    std::queue<std::pair<std::string, int>> q;
    q.push({"x", 7});
    REQUIRE(q.front().second == 7);
}

TEST_CASE("stdlib low header features", "[benchmark]") {
    // utility – move, swap, pair
    std::pair<std::string, std::vector<int>> p1{"first", {1, 2}};
    std::pair<std::string, std::vector<int>> p2 = std::move(p1);
    std::swap(p1, p2);
    REQUIRE(p2.second.size() == 2);

    // deque
    std::deque<std::pair<int, int>> dq;
    dq.emplace_back(1, 42);
    REQUIRE(dq.front().second == 42);

    // forward_list
    std::forward_list<std::pair<int, int>> fl{{1, 10}, {2, 20}, {3, 30}};
    REQUIRE(fl.begin()->first == 1);

    // list
    std::list<std::pair<int, int>> li{{4, 5}};
    li.emplace_back(6, 7);
    REQUIRE(li.back().first == 6);

    // set and unordered_set
    std::set<std::pair<int, int>> st{{3, 0}, {1, 0}, {2, 0}};
    REQUIRE(st.begin()->first == 1);
    std::unordered_set<std::string> uset{"seven", "eight", "nine"};
    REQUIRE(uset.count("eight") == 1);

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

    // atomic
    std::atomic<int> at{0};
    at.fetch_add(1);
    REQUIRE(at.load() == 1);

    // bitset
    std::bitset<4> bs("1010");
    REQUIRE(bs.count() == 2);

    // chrono & condition_variable
    std::condition_variable cv;
    bool ready = false;
    std::mutex cv_mx;
    std::thread waiter([&]() {
        std::unique_lock<std::mutex> lk(cv_mx);
        cv.wait_for(lk, std::chrono::milliseconds(10), [&] { return ready; });
    });
    {
        std::lock_guard<std::mutex> lk(cv_mx);
        ready = true;
    }
    cv.notify_one();
    waiter.join();

    // filesystem
    std::filesystem::path cur = std::filesystem::current_path();
    REQUIRE(cur.empty() == false);

    // functional / algorithm
    auto plus_fn = std::plus<int>{};
    REQUIRE(plus_fn(2, 3) == 5);
    std::vector<int> algo_vec = {5, 1, 4};
    std::sort(algo_vec.begin(), algo_vec.end());
    REQUIRE(algo_vec.front() == 1);

    // future / promise
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();
    prom.set_value(55);
    REQUIRE(fut.get() == 55);

    // ios goodbit (ios / iosfwd)
    REQUIRE(std::ios::goodbit == 0);

    // iterator back_inserter
    std::vector<int> it_vec;
    std::copy(local_arr.begin(), local_arr.end(), std::back_inserter(it_vec));
    REQUIRE(it_vec.size() == local_arr.size());

    // memory unique_ptr
    std::unique_ptr<int> uptr = std::make_unique<int>(99);
    REQUIRE(*uptr == 99);

    // new/delete
    int* raw_ptr = new int(123);
    REQUIRE(*raw_ptr == 123);
    delete raw_ptr;

    // ostream (use cout to stream to stringstream via rdbuf)
    std::ostringstream oss;
    std::ostream& os_ref = oss;
    os_ref << "hello";
    REQUIRE(oss.str() == "hello");

    // random
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 10);
    int rnd_val = dist(rng);
    REQUIRE(rnd_val >= 0);
    REQUIRE(rnd_val <= 10);

    // regex
    std::regex re("h.*o");
    REQUIRE(std::regex_match("hello", re));

    // streambuf
    std::stringbuf sbuf;
    sbuf.sputn("abc", 3);
    REQUIRE(sbuf.str() == "abc");

    // string_view
    std::string_view sv("abc");
    REQUIRE(sv.size() == 3);

    // valarray
    std::valarray<int> va = {1, 2, 3};
    REQUIRE(va.sum() == 6);

    // utility std::forward
    auto identity = [](auto&& x) -> decltype(auto) {
        return std::forward<decltype(x)>(x);
    };
    int id_val = identity(5);
    REQUIRE(id_val == 5);

    // iostream / istream explicit usage
    auto* cout_ptr = &std::cout;
    REQUIRE(cout_ptr != nullptr);
    std::istringstream iss("15 30");
    int a, b;
    iss >> a >> b;
    REQUIRE(a + b == 45);
} 