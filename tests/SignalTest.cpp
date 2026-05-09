#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
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
