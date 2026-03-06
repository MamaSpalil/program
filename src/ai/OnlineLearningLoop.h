#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// OnlineLearningLoop.h — Background RL Training Thread
// ─────────────────────────────────────────────────────────────────────────────
// Runs a dedicated thread that:
//   1. Polls TradeRepository for newly closed trades (→ Reward R_t = PnL).
//   2. Pushes experience tuples into a lock-free Replay Buffer (SPSCQueue).
//   3. When enough samples accumulate, assembles a mini-batch and calls
//      RLTradingAgent::train_step() to update weights via PPO + AdamW.
//
// The thread is cooperative-cancellable via std::atomic<bool> (C++17
// compatible — equivalent to std::jthread stop_token semantics).
// ─────────────────────────────────────────────────────────────────────────────

#include "RLTradingAgent.h"
#include "FeatureExtractor.h"
#include "SPSCQueue.h"
#include "../data/TradeRepository.h"
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>

namespace crypto { namespace ai {

// ── Configuration ────────────────────────────────────────────────────────────

struct OnlineLearningConfig {
    int    replayCapacityExp = 12;     ///< log2(capacity) — e.g. 12 → 4096 slots
    int    minReplaySize     = 256;    ///< Min samples before first training step
    int    trainIntervalMs   = 1000;   ///< Milliseconds between train attempts
    int    pollIntervalMs    = 500;    ///< Milliseconds between DB polls
    float  rewardScale       = 1.0f;   ///< Multiplier applied to raw PnL
    std::string modelSavePath = "config/rl_agent.pt";
    int    saveIntervalSteps = 100;    ///< Save checkpoint every N train steps
};

// ── Online Learning Loop ─────────────────────────────────────────────────────

class OnlineLearningLoop {
public:
    /// Construct the loop.  Does NOT start the thread — call start().
    /// @param agent  The PPO agent whose weights will be updated.
    /// @param feat   Feature extractor for constructing state vectors.
    /// @param trades Trade repository for reading closed trades (Reward).
    OnlineLearningLoop(RLTradingAgent& agent,
                       AIFeatureExtractor& feat,
                       TradeRepository* trades,
                       const OnlineLearningConfig& cfg = OnlineLearningConfig{});
    ~OnlineLearningLoop();

    OnlineLearningLoop(const OnlineLearningLoop&) = delete;
    OnlineLearningLoop& operator=(const OnlineLearningLoop&) = delete;

    // ── Thread control ───────────────────────────────────────────────────────

    /// Start the background training thread.
    void start();

    /// Request graceful stop and join the thread.
    void stop();

    /// Check if the training thread is running.
    bool running() const noexcept { return running_.load(std::memory_order_relaxed); }

    // ── Manual experience injection (from market thread) ─────────────────────

    /// Push an experience tuple into the lock-free replay buffer.
    /// Called from the producer (market / trading) thread.
    bool pushExperience(Experience&& exp);

    // ── State caching for RL correctness ────────────────────────────────────

    /// Capture and cache the current feature state for a trade.
    /// Call this when a position is opened so that pollTrades() can later
    /// retrieve the state at entry time instead of the stale current state.
    void captureStateForTrade(const std::string& tradeId);

    /// Retrieve a previously cached state vector for a trade.
    /// Returns empty vector if no state was cached for this trade ID.
    std::vector<float> getCachedState(const std::string& tradeId) const;

    // ── Statistics ───────────────────────────────────────────────────────────

    /// Total number of train_step() calls completed.
    uint64_t trainSteps() const noexcept { return trainSteps_.load(std::memory_order_relaxed); }

    /// Total number of experiences consumed from the buffer.
    uint64_t samplesConsumed() const noexcept { return samplesConsumed_.load(std::memory_order_relaxed); }

private:
    void threadFunc();
    void pollTrades();
    void tryTrain();

    RLTradingAgent&     agent_;
    AIFeatureExtractor& feat_;
    TradeRepository*    trades_;
    OnlineLearningConfig cfg_;

    // Lock-free replay buffer (4096 slots by default)
    SPSCQueue<Experience, 4096> replayBuffer_;

    // Local batch accumulator (owned by the consumer thread, no sync needed)
    std::vector<Experience> localBatch_;

    // Thread management
    std::thread       thread_;
    std::atomic<bool> stopFlag_{false};
    std::atomic<bool> running_{false};

    // Statistics (updated atomically by the training thread)
    std::atomic<uint64_t> trainSteps_{0};
    std::atomic<uint64_t> samplesConsumed_{0};

    // Track the last processed trade timestamp to avoid re-processing
    std::atomic<long long> lastTradeTime_{0};

    // State cache: maps trade ID → feature vector at entry time
    mutable std::mutex stateCacheMtx_;
    std::unordered_map<std::string, std::vector<float>> stateCache_;
};

}} // namespace crypto::ai
