#pragma once

#include "connection.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace sg {

template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    Signal()
        : m_state(std::make_shared<State>()) {
    }

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
    Signal(Signal&&) = delete;
    Signal& operator=(Signal&&) = delete;

    ~Signal() = default;

    template <typename Callable>
        requires std::invocable<Callable&, Args...> &&
                 std::same_as<std::invoke_result_t<Callable&, Args...>, void>
    [[nodiscard]] Connection connect(Callable&& callable) {
        return m_state->connect(Slot(std::forward<Callable>(callable)));
    }

    void emit(Args... args) {
        m_state->emit(std::forward<Args>(args)...);
    }

    void operator()(Args... args) {
        emit(std::forward<Args>(args)...);
    }

    void disconnect_all() noexcept {
        m_state->disconnect_all();
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_state->listener_count() == 0;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return m_state->listener_count();
    }

    [[nodiscard]] std::size_t listener_count() const noexcept {
        return size();
    }

private:
    struct SlotState {
        std::uint64_t id = 0;
        Slot slot;
        bool connected = true;
    };

    class State final : public detail::SignalStateBase {
    public:
        [[nodiscard]] Connection connect(Slot slot) {
            const auto id = m_next_id++;
            m_slots.push_back(std::make_shared<SlotState>(SlotState{
                .id = id,
                .slot = std::move(slot),
                .connected = true,
            }));
            ++m_active_count;
            return Connection(weak_from_this(), id);
        }

        void emit(Args... args) {
            ++m_emission_depth;
            const auto snapshot_size = m_slots.size();

            for (std::size_t index = 0; index < snapshot_size; ++index) {
                const auto& slot_state = m_slots[index];
                if (slot_state->connected) {
                    slot_state->slot(args...);
                }
            }

            --m_emission_depth;
            if (m_emission_depth == 0) {
                prune_disconnected();
            }
        }

        void disconnect(std::uint64_t id) override {
            const auto it = std::find_if(
                m_slots.begin(),
                m_slots.end(),
                [id](const std::shared_ptr<SlotState>& slot_state) {
                    return slot_state->id == id;
                });

            if (it == m_slots.end() || !(*it)->connected) {
                return;
            }

            (*it)->connected = false;
            --m_active_count;

            if (m_emission_depth == 0) {
                prune_disconnected();
            }
        }

        void disconnect_all() noexcept {
            if (m_active_count == 0) {
                return;
            }

            for (const auto& slot_state : m_slots) {
                slot_state->connected = false;
            }
            m_active_count = 0;

            if (m_emission_depth == 0) {
                prune_disconnected();
            }
        }

        [[nodiscard]] bool is_connected(std::uint64_t id) const override {
            const auto it = std::find_if(
                m_slots.begin(),
                m_slots.end(),
                [id](const std::shared_ptr<SlotState>& slot_state) {
                    return slot_state->id == id;
                });

            return it != m_slots.end() && (*it)->connected;
        }

        [[nodiscard]] std::size_t listener_count() const override {
            return m_active_count;
        }

    private:
        void prune_disconnected() {
            std::erase_if(
                m_slots,
                [](const std::shared_ptr<SlotState>& slot_state) {
                    return !slot_state->connected;
                });
        }

        std::vector<std::shared_ptr<SlotState>> m_slots;
        std::uint64_t m_next_id = 1;
        std::size_t m_active_count = 0;
        std::size_t m_emission_depth = 0;
    };

    std::shared_ptr<State> m_state;
};

} // namespace sg
