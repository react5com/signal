#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace sg::detail {

class SignalStateBase : public std::enable_shared_from_this<SignalStateBase> {
public:
    virtual ~SignalStateBase() = default;

    virtual void disconnect(std::uint64_t id) = 0;
    [[nodiscard]] virtual bool is_connected(std::uint64_t id) const = 0;
    [[nodiscard]] virtual std::size_t listener_count() const = 0;
};

} // namespace sg::detail
