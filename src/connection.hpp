#pragma once

#include "signal_base.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace sg {

class Connection {
public:
    Connection() = default;

    Connection(std::weak_ptr<detail::SignalStateBase> state, std::uint64_t id) noexcept
        : m_state(std::move(state))
        , m_id(id) {
    }

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&& other) noexcept
        : m_state(std::move(other.m_state))
        , m_id(std::exchange(other.m_id, 0)) {
    }

    Connection& operator=(Connection&& other) noexcept {
        if (this != &other) {
            disconnect();
            m_state = std::move(other.m_state);
            m_id = std::exchange(other.m_id, 0);
        }

        return *this;
    }

    ~Connection() {
        disconnect();
    }

    void disconnect() noexcept {
        if (m_id == 0) {
            return;
        }

        if (const auto state = m_state.lock()) {
            state->disconnect(m_id);
        }

        m_state.reset();
        m_id = 0;
    }

    [[nodiscard]] bool connected() const noexcept {
        if (m_id == 0) {
            return false;
        }

        const auto state = m_state.lock();
        return state != nullptr && state->is_connected(m_id);
    }

    explicit operator bool() const noexcept {
        return connected();
    }

private:
    friend class ScopedConnection;

    std::weak_ptr<detail::SignalStateBase> m_state;
    std::uint64_t m_id = 0;
};

} // namespace sg
