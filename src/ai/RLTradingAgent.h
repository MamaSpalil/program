#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// RLTradingAgent.h — PPO-based Reinforcement Learning Trading Agent
// ─────────────────────────────────────────────────────────────────────────────
// Encapsulates an Actor-Critic neural network for continuous RL on
// cryptocurrency markets. The actor outputs a stochastic policy over discrete
// actions {BUY, HOLD, SELL}; the critic estimates the state-value V(s).
//
// Key algorithms:
//   • Proximal Policy Optimization (PPO) with clipped surrogate objective
//   • Generalized Advantage Estimation (GAE-lambda)
//   • AdamW optimizer for weight updates
//
// All tensor operations use LibTorch (PyTorch C++ API).  When USE_LIBTORCH
// is not defined, a lightweight stub is compiled so the rest of the codebase
// links without modifications.
// ─────────────────────────────────────────────────────────────────────────────

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#ifdef USE_LIBTORCH
#include <torch/torch.h>
#endif

namespace crypto { namespace ai {

// ── Configuration ────────────────────────────────────────────────────────────

/// Hyper-parameters for the PPO agent.
struct PPOConfig {
    // Network architecture
    int    stateDim       = 128;   ///< Dimension of the input feature vector s_t
    int    hiddenDim      = 256;   ///< Width of hidden layers
    int    numActions      = 3;    ///< Discrete actions: 0=BUY, 1=HOLD, 2=SELL

    // PPO hyper-parameters
    float  gamma          = 0.99f; ///< Discount factor γ
    float  lambda         = 0.95f; ///< GAE-λ trace decay
    float  epsilon        = 0.2f;  ///< Clipping parameter ε
    float  entropyCoeff   = 0.01f; ///< Entropy bonus coefficient c₂
    float  valueLossCoeff = 0.5f;  ///< Value-loss coefficient c₁
    float  maxGradNorm    = 0.5f;  ///< Global gradient clipping

    // Optimizer
    float  learningRate   = 3e-4f; ///< AdamW learning rate
    float  weightDecay    = 1e-5f; ///< L2 regularisation
    int    ppoEpochs      = 4;    ///< Number of PPO optimisation epochs per batch
    int    miniBatchSize   = 64;   ///< Mini-batch size for gradient steps
};

// ── Experience tuple ─────────────────────────────────────────────────────────

/// A single transition (s_t, a_t, r_t, s_{t+1}, done, log π(a_t|s_t), V(s_t)).
struct Experience {
    std::vector<float> state;
    int                action     = 0;
    float              reward     = 0.0f;
    std::vector<float> nextState;
    bool               done       = false;
    float              logProb    = 0.0f;  ///< log π_old(a_t | s_t)
    float              value      = 0.0f;  ///< V(s_t) at collection time
};

/// A batch of transitions ready for PPO training.
struct ExperienceBatch {
    std::vector<Experience> transitions;
};

// ── Dual-mode enum ───────────────────────────────────────────────────────────

/// Operating mode for the agent.
enum class AgentMode : uint8_t {
    Fast  = 0,  ///< L2 Orderbook scalping (microsecond-level)
    Swing = 1   ///< Multi-timeframe trend following (CandleCache)
};

// ── Agent class ──────────────────────────────────────────────────────────────

/// PPO Reinforcement Learning agent with LibTorch backend.
class RLTradingAgent {
public:
    explicit RLTradingAgent(const PPOConfig& cfg = PPOConfig{});
    ~RLTradingAgent();

    RLTradingAgent(const RLTradingAgent&) = delete;
    RLTradingAgent& operator=(const RLTradingAgent&) = delete;

    // ── Inference ────────────────────────────────────────────────────────────

    /// Forward pass through actor-critic for a single state vector.
    /// Returns (action, log_prob, value_estimate).
    /// Designed for ultra-low latency: no allocations, torch::NoGradGuard.
    struct ActionResult {
        int   action;
        float logProb;
        float value;
    };
    ActionResult forward_pass(const std::vector<float>& stateVec) const;

#ifdef USE_LIBTORCH
    /// Tensor-based forward pass (avoids repeated vector→tensor copy).
    ActionResult forward_pass(const at::Tensor& state) const;
#endif

    // ── Training ─────────────────────────────────────────────────────────────

    /// Execute one PPO training step on a batch of collected transitions.
    /// Computes GAE advantages, clipped surrogate loss, value loss, entropy
    /// bonus, and applies AdamW gradient step.
    /// Returns (policy_loss, value_loss, entropy).
    struct TrainResult {
        float policyLoss;
        float valueLoss;
        float entropy;
    };
    TrainResult train_step(const ExperienceBatch& batch);

    // ── Serialisation ────────────────────────────────────────────────────────

    bool save(const std::string& path) const;
    bool load(const std::string& path);

    // ── Mode ─────────────────────────────────────────────────────────────────

    void      setMode(AgentMode m) noexcept { mode_ = m; }
    AgentMode getMode() const noexcept       { return mode_; }

    /// True when the model weights have been initialised or loaded.
    bool ready() const noexcept { return ready_; }

    const PPOConfig& config() const noexcept { return cfg_; }

private:
    PPOConfig  cfg_;
    AgentMode  mode_{AgentMode::Fast};
    bool       ready_{false};

#ifdef USE_LIBTORCH
    // ── Actor-Critic network (shared backbone) ──────────────────────────────
    struct ActorCriticImpl;
    std::shared_ptr<ActorCriticImpl> net_;
    std::unique_ptr<torch::optim::AdamW> optimizer_;
    torch::Device device_;

    // Internal helpers
    at::Tensor computeGAE(const at::Tensor& rewards,
                          const at::Tensor& values,
                          const at::Tensor& dones) const;
#endif
};

}} // namespace crypto::ai
