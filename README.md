# signal

`signal` is a small header-only C++20 signal/slot library in the `sg` namespace. It provides ordered listener dispatch, RAII disconnection, and safe mutation during emit, including self-disconnect, disconnecting other listeners, and nested emits.

## Requirements

- C++20
- Header-only
- Single-threaded use only

This version does not provide thread safety, combiners, result aggregation, slot grouping, or blocking.

## Integration

### CMake

```cmake
find_package(signal CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE sg::signal)
```

### Direct include

Copy the headers from `src/` into your include path and include:

```cpp
#include <sg.hpp>
```

## Basic usage

```cpp
#include <sg.hpp>
#include <iostream>

int main() {
    sg::Signal<int> value_changed;

    sg::ScopedConnection log_connection(
        value_changed.connect([](int value) {
            std::cout << "value: " << value << '\n';
        }));

    auto oneshot = value_changed.connect([&](int value) {
        std::cout << "first and only: " << value << '\n';
    });

    value_changed.emit(7);
    oneshot.disconnect();
    value_changed(9);
}
```

## Behavior notes

- Listeners run in registration order.
- Disconnecting the current listener or another listener during emit is safe.
- New listeners added during an emit are not invoked by that active emit; they participate in later emits.
- Nested emits are supported. Each emit observes the listener list visible when that emit begins.
- `sg::Connection` is move-only and disconnects on destruction.
- `sg::ScopedConnection` wraps a `Connection` and disconnects automatically unless `release()` transfers ownership.

## Public API

- `sg::Signal<Args...>`
- `sg::Connection`
- `sg::ScopedConnection`

Core methods:

- `connect(...)`
- `emit(...)`
- `operator()(...)`
- `disconnect_all()`
- `empty()`
- `size()`
- `listener_count()`
- `disconnect()`
- `connected()`
- `reset()`
- `release()`
