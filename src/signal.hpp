#pragma once

#include "connection.hpp"

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
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
        SlotState(std::uint64_t id_value, Slot slot_value)
            : id(id_value)
            , slot(std::move(slot_value))
            , connected(true) {
        }

        std::uint64_t id = 0;
        Slot slot;
        std::atomic<bool> connected;
    };

    class State final : public detail::SignalStateBase {
    public:
        [[nodiscard]] Connection connect(Slot slot) {
            const std::lock_guard lock(m_mutex);
            const auto id = m_next_id++;
            m_slots.push_back(std::make_shared<SlotState>(id, std::move(slot)));
            ++m_active_count;
            return Connection(weak_from_this(), id);
        }

        void emit(Args... args) {
            struct EmissionGuard final {
                explicit EmissionGuard(State& state) noexcept
                    : state(state) {
                    state.m_emission_depth.fetch_add(1, std::memory_order_relaxed);
                }

                ~EmissionGuard() {
                    if (state.m_emission_depth.fetch_sub(1, std::memory_order_relaxed) == 1) {
                        const std::lock_guard lock(state.m_mutex);
                        state.prune_disconnected_locked();
                    }
                }

                State& state;
            } emission_guard(*this);

            std::vector<std::shared_ptr<SlotState>> snapshot;
            {
                const std::lock_guard lock(m_mutex);
                snapshot = m_slots;
            }

            for (const auto& slot_state : snapshot) {
                if (slot_state->connected.load(std::memory_order_acquire)) {
                    slot_state->slot(dispatch_arg<Args>(args)...);
                }
            }
        }

        void disconnect(std::uint64_t id) override {
            std::lock_guard lock(m_mutex);
            const auto it = std::find_if(
                m_slots.begin(),
                m_slots.end(),
                [id](const std::shared_ptr<SlotState>& slot_state) {
                    return slot_state->id == id;
                });

            if (it == m_slots.end() ||
                !(*it)->connected.load(std::memory_order_acquire)) {
                return;
            }

            bool expected = true;
            if (!(*it)->connected.compare_exchange_strong(
                    expected,
                    false,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                return;
            }
            --m_active_count;

            if (m_emission_depth.load(std::memory_order_relaxed) == 0) {
                prune_disconnected_locked();
            }
        }

        void disconnect_all() noexcept {
            std::lock_guard lock(m_mutex);
            if (m_active_count == 0) {
                return;
            }

            for (const auto& slot_state : m_slots) {
                slot_state->connected.store(false, std::memory_order_release);
            }
            m_active_count = 0;

            if (m_emission_depth.load(std::memory_order_relaxed) == 0) {
                prune_disconnected_locked();
            }
        }

        [[nodiscard]] bool is_connected(std::uint64_t id) const override {
            const std::lock_guard lock(m_mutex);
            const auto it = std::find_if(
                m_slots.begin(),
                m_slots.end(),
                [id](const std::shared_ptr<SlotState>& slot_state) {
                    return slot_state->id == id;
                });

            return it != m_slots.end() &&
                   (*it)->connected.load(std::memory_order_acquire);
        }

        [[nodiscard]] std::size_t listener_count() const override {
            const std::lock_guard lock(m_mutex);
            return m_active_count;
        }

    private:
        template <typename Arg>
        static decltype(auto) dispatch_arg(Arg& arg) noexcept {
            if constexpr (std::is_rvalue_reference_v<Arg>) {
                return std::move(arg);
            } else {
                return (arg);
            }
        }

        void prune_disconnected_locked() {
            std::erase_if(
                m_slots,
                [](const std::shared_ptr<SlotState>& slot_state) {
                    return !slot_state->connected.load(std::memory_order_acquire);
                });
        }

        mutable std::mutex m_mutex;
        std::vector<std::shared_ptr<SlotState>> m_slots;
        std::uint64_t m_next_id = 1;
        std::size_t m_active_count = 0;
        std::atomic<std::size_t> m_emission_depth{0};
    };

    std::shared_ptr<State> m_state;
};

} // namespace sg
