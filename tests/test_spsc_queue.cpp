#include <catch2/catch_test_macros.hpp>
#include <lobster/spsc_queue.hpp>

#include <atomic>
#include <thread>
#include <vector>

TEST_CASE("SpscQueue single-thread push/pop preserves order", "[spsc]") {
    lobster::SpscQueue<int> q{8};
    REQUIRE(q.empty());
    REQUIRE(q.try_push(1));
    REQUIRE(q.try_push(2));
    REQUIRE(q.try_push(3));

    auto a = q.try_pop();
    auto b = q.try_pop();
    auto c = q.try_pop();
    auto d = q.try_pop();

    REQUIRE(a.has_value());
    REQUIRE(*a == 1);
    REQUIRE(b.has_value());
    REQUIRE(*b == 2);
    REQUIRE(c.has_value());
    REQUIRE(*c == 3);
    REQUIRE_FALSE(d.has_value());
}

TEST_CASE("SpscQueue rejects push when full", "[spsc]") {
    lobster::SpscQueue<int> q{4};  // rounded up — capacity 4, usable 3
    REQUIRE(q.try_push(1));
    REQUIRE(q.try_push(2));
    REQUIRE(q.try_push(3));
    REQUIRE_FALSE(q.try_push(4));  // full
    auto a = q.try_pop();
    REQUIRE(a.has_value());
    REQUIRE(q.try_push(4));  // space again
}

TEST_CASE("SpscQueue producer/consumer threads transfer all values", "[spsc][slow]") {
    constexpr int N = 100'000;
    lobster::SpscQueue<int> q{1024};
    std::atomic<bool> producer_done{false};
    std::vector<int> received;
    received.reserve(N);

    std::thread producer([&] {
        for (int i = 0; i < N; ++i) {
            while (!q.try_push(i)) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&] {
        while (received.size() < static_cast<std::size_t>(N)) {
            auto v = q.try_pop();
            if (v.has_value()) {
                received.push_back(*v);
            } else if (producer_done.load(std::memory_order_acquire)) {
                continue;  // drain remaining
            }
        }
    });

    producer.join();
    consumer.join();

    REQUIRE(received.size() == static_cast<std::size_t>(N));
    for (int i = 0; i < N; ++i) {
        REQUIRE(received[static_cast<std::size_t>(i)] == i);
    }
}
