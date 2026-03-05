#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// SPSCQueue.h — Lock-free Single-Producer Single-Consumer Ring Buffer
// ─────────────────────────────────────────────────────────────────────────────
// Cache-line-padded, no dynamic allocations on push/pop.
// Used to pass experience tuples from the market thread to the AI trainer.
// ─────────────────────────────────────────────────────────────────────────────

#include <atomic>
#include <cstddef>
#include <array>
#include <optional>

namespace crypto { namespace ai {

// Portable cache-line size — use a fixed constant to avoid ABI instability
// warnings from std::hardware_destructive_interference_size.
#ifndef CACHE_LINE_SIZE
    inline constexpr std::size_t CACHE_LINE_SIZE = 64;
#endif

/// Fixed-capacity lock-free SPSC (Single Producer, Single Consumer) ring buffer.
/// @tparam T      Element type (must be trivially copyable or nothrow move).
/// @tparam Capacity  Number of slots; must be a power of two.
template <typename T, std::size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "SPSCQueue capacity must be a power of two");
    static_assert(Capacity >= 2, "SPSCQueue capacity must be at least 2");

public:
    SPSCQueue() noexcept : head_(0), tail_(0) {}

    /// Try to enqueue an element. Returns false if the queue is full.
    /// Must be called only from the producer thread.
    bool try_push(const T& item) noexcept {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t next = (h + 1) & MASK;
        // If next slot equals tail, the buffer is full
        if (next == tail_.load(std::memory_order_acquire))
            return false;
        buf_[h] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    /// Try to enqueue via move. Returns false if the queue is full.
    bool try_push(T&& item) noexcept {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t next = (h + 1) & MASK;
        if (next == tail_.load(std::memory_order_acquire))
            return false;
        buf_[h] = std::move(item);
        head_.store(next, std::memory_order_release);
        return true;
    }

    /// Try to dequeue an element. Returns std::nullopt if the queue is empty.
    /// Must be called only from the consumer thread.
    std::optional<T> try_pop() noexcept {
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire))
            return std::nullopt;  // empty
        T item = std::move(buf_[t]);
        tail_.store((t + 1) & MASK, std::memory_order_release);
        return item;
    }

    /// Non-blocking size estimate (may be stale).
    std::size_t size_approx() const noexcept {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        return (h - t) & MASK;
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) ==
               tail_.load(std::memory_order_relaxed);
    }

    static constexpr std::size_t capacity() noexcept { return Capacity - 1; }

private:
    static constexpr std::size_t MASK = Capacity - 1;

    // Pad head and tail onto separate cache lines to avoid false sharing
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> head_;
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> tail_;
    alignas(CACHE_LINE_SIZE) std::array<T, Capacity>  buf_;
};

}} // namespace crypto::ai
