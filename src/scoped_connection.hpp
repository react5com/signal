#pragma once

#include "connection.hpp"

#include <utility>

namespace sg {

class ScopedConnection {
public:
    ScopedConnection() = default;

    explicit ScopedConnection(Connection connection) noexcept
        : m_connection(std::move(connection)) {
    }

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    ScopedConnection(ScopedConnection&& other) noexcept
        : m_connection(std::move(other.m_connection)) {
    }

    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if (this != &other) {
            reset();
            m_connection = std::move(other.m_connection);
        }

        return *this;
    }

    ~ScopedConnection() {
        reset();
    }

    void reset() noexcept {
        m_connection.disconnect();
    }

    void reset(Connection connection) noexcept {
        m_connection.disconnect();
        m_connection = std::move(connection);
    }

    [[nodiscard]] Connection release() noexcept {
        Connection released = std::move(m_connection);
        m_connection = Connection{};
        return released;
    }

    [[nodiscard]] bool connected() const noexcept {
        return m_connection.connected();
    }

    explicit operator bool() const noexcept {
        return connected();
    }

private:
    Connection m_connection;
};

} // namespace sg
