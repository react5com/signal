#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "sg.hpp"

TEST_CASE("listeners fire in registration order") {
    sg::Signal<int> signal;
    std::vector<int> values;

    auto first = signal.connect([&](int value) { values.push_back(value); });
    auto second = signal.connect([&](int value) { values.push_back(value * 10); });

    signal.emit(3);

    REQUIRE(values == std::vector<int>{3, 30});
    REQUIRE(first.connected());
    REQUIRE(second.connected());
}

TEST_CASE("by-value payload is preserved for every listener") {
    sg::Signal<std::string> signal;
    std::vector<std::string> values;

    auto first = signal.connect([&](std::string value) { values.push_back(std::move(value)); });
    auto second = signal.connect([&](std::string value) { values.push_back(std::move(value)); });

    signal.emit(std::string("payload"));

    REQUIRE(values == std::vector<std::string>{"payload", "payload"});
    REQUIRE(first.connected());
    REQUIRE(second.connected());
}

TEST_CASE("rvalue-reference payload stays an rvalue at dispatch") {
    sg::Signal<std::string&&> signal;
    std::string observed;

    auto connection = signal.connect([&](std::string&& value) {
        observed = std::move(value);
    });

    auto payload = std::string("payload");
    signal.emit(std::move(payload));

    REQUIRE(connection.connected());
    REQUIRE(observed == "payload");
}

TEST_CASE("connection is move only") {
    sg::Signal<> signal;
    int calls = 0;

    auto original = signal.connect([&]() { ++calls; });
    auto moved = std::move(original);

    REQUIRE_FALSE(original.connected());
    REQUIRE(moved.connected());

    signal();
    REQUIRE(calls == 1);
}

TEST_CASE("connection destructor disconnects") {
    sg::Signal<> signal;
    int calls = 0;

    {
        auto connection = signal.connect([&]() { ++calls; });
        REQUIRE(connection.connected());
    }

    signal.emit();
    REQUIRE(calls == 0);
    REQUIRE(signal.empty());
}

TEST_CASE("explicit disconnect is idempotent") {
    sg::Signal<> signal;
    int calls = 0;

    auto connection = signal.connect([&]() { ++calls; });
    connection.disconnect();
    connection.disconnect();

    signal.emit();

    REQUIRE(calls == 0);
    REQUIRE_FALSE(connection.connected());
    REQUIRE(signal.empty());
}

TEST_CASE("disconnect_all clears listeners") {
    sg::Signal<> signal;
    int calls = 0;

    auto first = signal.connect([&]() { ++calls; });
    auto second = signal.connect([&]() { ++calls; });

    signal.disconnect_all();
    signal.emit();

    REQUIRE(calls == 0);
    REQUIRE(signal.empty());
    REQUIRE(signal.listener_count() == 0);
    REQUIRE_FALSE(first.connected());
    REQUIRE_FALSE(second.connected());
}

TEST_CASE("size and empty reflect active listeners") {
    sg::Signal<int> signal;

    REQUIRE(signal.empty());
    REQUIRE(signal.size() == 0);

    auto first = signal.connect([](int) {});
    REQUIRE_FALSE(signal.empty());
    REQUIRE(signal.size() == 1);

    auto second = signal.connect([](int) {});
    REQUIRE(signal.listener_count() == 2);

    first.disconnect();
    REQUIRE(signal.size() == 1);

    second.disconnect();
    REQUIRE(signal.empty());
}

TEST_CASE("listener can disconnect itself during emit") {
    sg::Signal<> signal;
    int calls = 0;
    std::unique_ptr<sg::Connection> self;

    self = std::make_unique<sg::Connection>(signal.connect([&]() {
        ++calls;
        self->disconnect();
    }));

    signal.emit();
    signal.emit();

    REQUIRE(calls == 1);
    REQUIRE_FALSE(self->connected());
}

TEST_CASE("listener can disconnect another listener during emit") {
    sg::Signal<> signal;
    std::vector<std::string> calls;

    sg::Connection second;
    auto first = signal.connect([&]() {
        calls.push_back("first");
        second.disconnect();
    });
    second = signal.connect([&]() { calls.push_back("second"); });
    auto third = signal.connect([&]() { calls.push_back("third"); });

    signal.emit();
    signal.emit();

    REQUIRE(calls == std::vector<std::string>{"first", "third", "first", "third"});
    REQUIRE(first.connected());
    REQUIRE_FALSE(second.connected());
    REQUIRE(third.connected());
}

TEST_CASE("new listeners added during emit join the next emit only") {
    sg::Signal<int> signal;
    std::vector<int> values;
    bool attached = false;
    std::vector<sg::Connection> retained;

    auto first = signal.connect([&](int value) {
        values.push_back(value);
        if (!attached) {
            attached = true;
            retained.push_back(
                signal.connect([&](int nested_value) { values.push_back(nested_value * 10); }));
        }
    });

    signal.emit(1);
    signal.emit(2);

    REQUIRE(values == std::vector<int>{1, 2, 20});
    REQUIRE(first.connected());
    REQUIRE(signal.size() == 2);
}

TEST_CASE("nested emits are reentrant safe") {
    sg::Signal<int> signal;
    std::vector<int> values;
    bool nested = false;

    auto first = signal.connect([&](int value) {
        values.push_back(value);
        if (!nested) {
            nested = true;
            signal.emit(value + 1);
        }
    });
    auto second = signal.connect([&](int value) { values.push_back(value * 10); });

    signal.emit(1);

    REQUIRE(values == std::vector<int>{1, 2, 20, 10});
    REQUIRE(first.connected());
    REQUIRE(second.connected());
}

TEST_CASE("handles remain safe after signal destruction") {
    auto signal = std::make_unique<sg::Signal<>>();
    auto connection = signal->connect([]() {});

    REQUIRE(connection.connected());
    signal.reset();

    REQUIRE_FALSE(connection.connected());
    connection.disconnect();
    REQUIRE_FALSE(connection.connected());
}

TEST_CASE("scoped connection disconnects on destruction and release transfers ownership") {
    sg::Signal<> signal;
    int calls = 0;

    {
        sg::ScopedConnection scoped(signal.connect([&]() { ++calls; }));
        REQUIRE(scoped.connected());
    }

    signal.emit();
    REQUIRE(calls == 0);

    sg::ScopedConnection scoped(signal.connect([&]() { ++calls; }));
    auto released = scoped.release();

    REQUIRE_FALSE(scoped.connected());
    REQUIRE(released.connected());

    signal.emit();
    REQUIRE(calls == 1);

    released.disconnect();
    signal.emit();
    REQUIRE(calls == 1);
}

TEST_CASE("umbrella include works in a minimal translation unit") {
    sg::Signal<int> signal;
    auto connection = signal.connect([](int) {});

    REQUIRE(connection.connected());
    REQUIRE(signal.size() == 1);
}

TEST_CASE("concurrent emits invoke listeners exactly once per emission") {
    sg::Signal<int> signal;
    std::atomic<int> total = 0;

    auto connection = signal.connect([&](int value) {
        total.fetch_add(value, std::memory_order_relaxed);
    });

    constexpr int thread_count = 8;
    constexpr int emits_per_thread = 1000;
    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    for (int index = 0; index < thread_count; ++index) {
        threads.emplace_back([&]() {
            for (int emit_index = 0; emit_index < emits_per_thread; ++emit_index) {
                signal.emit(1);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    REQUIRE(connection.connected());
    REQUIRE(total.load(std::memory_order_relaxed) == thread_count * emits_per_thread);
}

TEST_CASE("concurrent connect and disconnect remain safe during emit") {
    sg::Signal<int> signal;
    std::atomic<int> total = 0;

    auto stable = signal.connect([&](int value) {
        total.fetch_add(value, std::memory_order_relaxed);
    });

    constexpr int worker_count = 4;
    constexpr int iterations = 200;
    std::vector<std::thread> threads;
    threads.reserve(worker_count + 1);

    threads.emplace_back([&]() {
        for (int iteration = 0; iteration < worker_count * iterations; ++iteration) {
            signal.emit(1);
        }
    });

    for (int worker = 0; worker < worker_count; ++worker) {
        threads.emplace_back([&]() {
            for (int iteration = 0; iteration < iterations; ++iteration) {
                auto transient = signal.connect([&](int value) {
                    total.fetch_add(value, std::memory_order_relaxed);
                });
                if ((iteration % 3) == 0) {
                    transient.disconnect();
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    signal.disconnect_all();

    REQUIRE(stable.connected() == false);
    REQUIRE(signal.empty());
    REQUIRE(signal.listener_count() == 0);
    REQUIRE(total.load(std::memory_order_relaxed) >= worker_count * iterations);
}
