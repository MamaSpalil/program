// ─────────────────────────────────────────────────────────────────────────────
// OnlineLearningLoop.cpp — Background RL Training Thread Implementation
// ─────────────────────────────────────────────────────────────────────────────

#include "OnlineLearningLoop.h"
#include "../core/Logger.h"

#include <chrono>
#include <algorithm>
#include <cmath>

namespace crypto { namespace ai {

// ── Constructor ──────────────────────────────────────────────────────────────

OnlineLearningLoop::OnlineLearningLoop(
        RLTradingAgent& agent,
        AIFeatureExtractor& feat,
        TradeRepository* trades,
        const OnlineLearningConfig& cfg)
    : agent_(agent), feat_(feat), trades_(trades), cfg_(cfg) {
    // Pre-allocate the local batch to avoid allocations in the hot loop
    localBatch_.reserve(cfg_.minReplaySize * 2);
}

OnlineLearningLoop::~OnlineLearningLoop() {
    stop();
}

// ── Thread control ───────────────────────────────────────────────────────────

void OnlineLearningLoop::start() {
    if (running_.load(std::memory_order_relaxed)) return;

    stopFlag_.store(false, std::memory_order_relaxed);
    thread_ = std::thread(&OnlineLearningLoop::threadFunc, this);
    running_.store(true, std::memory_order_release);

    Logger::get()->info("[OnlineLearning] Training thread started "
                        "(replay_min={}, interval={}ms)",
                        cfg_.minReplaySize, cfg_.trainIntervalMs);
}

void OnlineLearningLoop::stop() {
    if (!running_.load(std::memory_order_relaxed)) return;

    stopFlag_.store(true, std::memory_order_release);
    if (thread_.joinable()) thread_.join();
    running_.store(false, std::memory_order_release);

    Logger::get()->info("[OnlineLearning] Training thread stopped "
                        "(total_steps={}, samples={})",
                        trainSteps_.load(), samplesConsumed_.load());
}

// ── Experience injection ─────────────────────────────────────────────────────

bool OnlineLearningLoop::pushExperience(Experience&& exp) {
    return replayBuffer_.try_push(std::move(exp));
}

// ── Main thread function ─────────────────────────────────────────────────────

void OnlineLearningLoop::threadFunc() {
    Logger::get()->debug("[OnlineLearning] Thread entered");

    using Clock = std::chrono::steady_clock;
    auto lastTrain = Clock::now();
    auto lastPoll  = Clock::now();

    while (!stopFlag_.load(std::memory_order_acquire)) {
        auto now = Clock::now();

        // ── Poll TradeRepository for new closed trades ───────────────────────
        auto msSincePoll = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastPoll).count();
        if (msSincePoll >= cfg_.pollIntervalMs) {
            pollTrades();
            lastPoll = now;
        }

        // ── Drain the SPSC queue into the local batch ────────────────────────
        while (auto exp = replayBuffer_.try_pop()) {
            localBatch_.push_back(std::move(*exp));
            samplesConsumed_.fetch_add(1, std::memory_order_relaxed);
        }

        // ── Train if enough samples and interval elapsed ─────────────────────
        auto msSinceTrain = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastTrain).count();
        if (static_cast<int>(localBatch_.size()) >= cfg_.minReplaySize &&
            msSinceTrain >= cfg_.trainIntervalMs) {
            tryTrain();
            lastTrain = Clock::now();
        }

        // Sleep briefly to avoid busy-spinning (1ms is fine for a training thread)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    Logger::get()->debug("[OnlineLearning] Thread exiting");
}

// ── Poll TradeRepository ─────────────────────────────────────────────────────
//
// Reads recently closed trades to compute Reward R_t = PnL * rewardScale.
// For each new trade, we generate a synthetic experience tuple with the
// reward derived from the trade's realised P&L.
// ─────────────────────────────────────────────────────────────────────────────

void OnlineLearningLoop::pollTrades() {
    if (!trades_) return;

    try {
        // Fetch recent trade history (limit 50)
        auto history = trades_->getHistory("", 50);

        for (const auto& t : history) {
            // Skip trades we have already processed
            if (t.exitTime <= lastTradeTime_) continue;
            if (t.status != "closed") continue;

            // R_t = PnL * rewardScale
            float reward = static_cast<float>(t.pnl) * cfg_.rewardScale;

            // Build a minimal experience tuple from the trade.
            // The state vector comes from the feature extractor's current state
            // (this is an approximation — in a full system the state at trade
            // time would be stored alongside the order).
            Experience exp;
            exp.state    = feat_.extract(agent_.getMode());
            exp.action   = (t.side == "buy") ? 0 : 2; // BUY=0, SELL=2
            exp.reward   = reward;
            exp.nextState = exp.state; // approximation for online setting
            exp.done     = true;       // trade is completed
            exp.logProb  = 0.0f;      // unknown for historical trades
            exp.value    = 0.0f;

            localBatch_.push_back(std::move(exp));
            samplesConsumed_.fetch_add(1, std::memory_order_relaxed);

            if (t.exitTime > lastTradeTime_)
                lastTradeTime_ = t.exitTime;
        }
    } catch (const std::exception& e) {
        Logger::get()->warn("[OnlineLearning] pollTrades error: {}", e.what());
    }
}

// ── Train step ───────────────────────────────────────────────────────────────
//
// Assembles a batch from the local accumulator and calls the agent's PPO
// train_step(). After training, the oldest experiences are discarded to
// keep memory bounded.
// ─────────────────────────────────────────────────────────────────────────────

void OnlineLearningLoop::tryTrain() {
    if (localBatch_.empty()) return;

    // Assemble the batch (use all accumulated experiences)
    ExperienceBatch batch;
    batch.transitions = localBatch_;

    auto result = agent_.train_step(batch);

    uint64_t steps = trainSteps_.fetch_add(1, std::memory_order_relaxed) + 1;

    Logger::get()->info("[OnlineLearning] step={} batch_size={} "
                        "policy_loss={:.4f} value_loss={:.4f} entropy={:.4f}",
                        steps, localBatch_.size(),
                        result.policyLoss, result.valueLoss, result.entropy);

    // Keep only the most recent quarter of experiences to maintain freshness
    // while retaining some history for stability
    if (localBatch_.size() > static_cast<size_t>(cfg_.minReplaySize)) {
        size_t keep = localBatch_.size() / 4;
        if (keep < static_cast<size_t>(cfg_.minReplaySize / 2))
            keep = std::min(localBatch_.size(),
                            static_cast<size_t>(cfg_.minReplaySize / 2));
        localBatch_.erase(localBatch_.begin(),
                          localBatch_.begin() + (localBatch_.size() - keep));
    }

    // Periodic checkpoint save
    if (steps % cfg_.saveIntervalSteps == 0) {
        agent_.save(cfg_.modelSavePath);
    }
}

}} // namespace crypto::ai
