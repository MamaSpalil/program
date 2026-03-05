// ─────────────────────────────────────────────────────────────────────────────
// RLTradingAgent.cpp — PPO Reinforcement Learning Agent Implementation
// ─────────────────────────────────────────────────────────────────────────────

#include "RLTradingAgent.h"
#include "../core/Logger.h"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>

namespace crypto { namespace ai {

// ═════════════════════════════════════════════════════════════════════════════
// LibTorch-backed implementation
// ═════════════════════════════════════════════════════════════════════════════
#ifdef USE_LIBTORCH

// ── Actor-Critic Module ──────────────────────────────────────────────────────
// Shared-backbone architecture:
//   backbone : Linear(stateDim → hidden) → ReLU → Linear(hidden → hidden) → ReLU
//   actor    : Linear(hidden → numActions)  →  softmax  →  π(a|s)
//   critic   : Linear(hidden → 1)           →  V(s)

struct RLTradingAgent::ActorCriticImpl : torch::nn::Module {
    torch::nn::Linear backbone1{nullptr};
    torch::nn::Linear backbone2{nullptr};
    torch::nn::Linear actorHead{nullptr};
    torch::nn::Linear criticHead{nullptr};
    torch::nn::LayerNorm ln1{nullptr};
    torch::nn::LayerNorm ln2{nullptr};

    ActorCriticImpl(int stateDim, int hiddenDim, int numActions) {
        // Shared backbone — two dense layers with LayerNorm + ReLU
        backbone1  = register_module("backbone1",
            torch::nn::Linear(stateDim, hiddenDim));
        ln1        = register_module("ln1",
            torch::nn::LayerNorm(torch::nn::LayerNormOptions({hiddenDim})));
        backbone2  = register_module("backbone2",
            torch::nn::Linear(hiddenDim, hiddenDim));
        ln2        = register_module("ln2",
            torch::nn::LayerNorm(torch::nn::LayerNormOptions({hiddenDim})));

        // Actor head — outputs logits over discrete actions
        actorHead  = register_module("actor",
            torch::nn::Linear(hiddenDim, numActions));

        // Critic head — outputs scalar state-value V(s)
        criticHead = register_module("critic",
            torch::nn::Linear(hiddenDim, 1));

        // Orthogonal initialisation (PPO best practice)
        for (auto& p : this->parameters()) {
            if (p.dim() >= 2) {
                torch::nn::init::orthogonal_(p, std::sqrt(2.0));
            }
        }
        // Actor head uses smaller gain for exploration
        torch::nn::init::orthogonal_(actorHead->weight, 0.01);
        // Critic head uses gain = 1
        torch::nn::init::orthogonal_(criticHead->weight, 1.0);
    }

    /// Forward through the shared backbone.
    /// @return (action_logits [B, numActions], values [B, 1])
    std::pair<torch::Tensor, torch::Tensor> forward(torch::Tensor x) {
        // Backbone
        auto h = torch::relu(ln1->forward(backbone1->forward(x)));
        h      = torch::relu(ln2->forward(backbone2->forward(h)));

        // Actor: log-softmax for numerical stability
        auto logits = actorHead->forward(h);  // [B, numActions]

        // Critic: single scalar
        auto value  = criticHead->forward(h); // [B, 1]

        return {logits, value};
    }
};

// ── Constructor / Destructor ─────────────────────────────────────────────────

RLTradingAgent::RLTradingAgent(const PPOConfig& cfg)
    : cfg_(cfg),
      device_(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU) {
    net_ = std::make_shared<ActorCriticImpl>(
        cfg_.stateDim, cfg_.hiddenDim, cfg_.numActions);
    net_->to(device_);

    // AdamW optimizer with weight decay (L2 regularisation)
    optimizer_ = std::make_unique<torch::optim::AdamW>(
        net_->parameters(),
        torch::optim::AdamWOptions(cfg_.learningRate)
            .weight_decay(cfg_.weightDecay));

    ready_ = true;
    Logger::get()->info("[RLAgent] Initialised on {} | state_dim={} hidden={} actions={}",
        device_ == torch::kCUDA ? "CUDA" : "CPU",
        cfg_.stateDim, cfg_.hiddenDim, cfg_.numActions);
}

RLTradingAgent::~RLTradingAgent() = default;

// ── Inference (vector overload) ──────────────────────────────────────────────

RLTradingAgent::ActionResult
RLTradingAgent::forward_pass(const std::vector<float>& stateVec) const {
    // Convert vector → 1-D tensor → [1, stateDim]
    auto t = torch::from_blob(
        const_cast<float*>(stateVec.data()),
        {static_cast<int64_t>(stateVec.size())},
        torch::kFloat32).clone().unsqueeze(0).to(device_);
    return forward_pass(t);
}

// ── Inference (tensor overload) ──────────────────────────────────────────────

RLTradingAgent::ActionResult
RLTradingAgent::forward_pass(const at::Tensor& state) const {
    if (!ready_) return {1, 0.0f, 0.0f}; // default: HOLD

    torch::NoGradGuard noGrad;
    net_->eval();

    auto [logits, value] = net_->forward(state);

    // Categorical sampling from the policy π(a|s)
    auto probs   = torch::softmax(logits, /*dim=*/1);         // [1, A]
    auto dist    = torch::multinomial(probs, /*num_samples=*/1); // [1, 1]
    int action   = dist.item<int>();

    // log π(a|s) for importance-ratio computation later
    auto logProbs = torch::log_softmax(logits, /*dim=*/1);    // [1, A]
    float lp      = logProbs[0][action].item<float>();
    float v       = value[0][0].item<float>();

    return {action, lp, v};
}

// ── Training step (PPO) ─────────────────────────────────────────────────────
//
// PPO Clipped Surrogate Objective:
//
//   L^{CLIP}(θ) = E_t[ min( r_t(θ) * Â_t ,
//                            clip(r_t(θ), 1-ε, 1+ε) * Â_t ) ]
//
// where r_t(θ) = π_θ(a_t|s_t) / π_{θ_old}(a_t|s_t)
//
// Total loss = -L^{CLIP} + c₁ * L^{VF} - c₂ * S[π_θ]
//   L^{VF}   = MSE( V_θ(s_t), V_target )
//   S[π_θ]   = entropy bonus for exploration
// ─────────────────────────────────────────────────────────────────────────────

RLTradingAgent::TrainResult
RLTradingAgent::train_step(const ExperienceBatch& batch) {
    if (!ready_ || batch.transitions.empty()) return {0, 0, 0};

    const int N = static_cast<int>(batch.transitions.size());

    // ── 1. Vectorise the batch into tensors ─────────────────────────────────
    auto states    = torch::zeros({N, cfg_.stateDim}, torch::kFloat32);
    auto actions   = torch::zeros({N}, torch::kLong);
    auto rewards   = torch::zeros({N}, torch::kFloat32);
    auto dones     = torch::zeros({N}, torch::kFloat32);
    auto oldLogP   = torch::zeros({N}, torch::kFloat32);
    auto oldValues = torch::zeros({N}, torch::kFloat32);

    for (int i = 0; i < N; ++i) {
        const auto& t = batch.transitions[i];
        for (int j = 0; j < cfg_.stateDim && j < static_cast<int>(t.state.size()); ++j)
            states[i][j] = t.state[j];
        actions[i]   = t.action;
        rewards[i]   = t.reward;
        dones[i]     = t.done ? 1.0f : 0.0f;
        oldLogP[i]   = t.logProb;
        oldValues[i] = t.value;
    }

    // Move to compute device
    states    = states.to(device_);
    actions   = actions.to(device_);
    rewards   = rewards.to(device_);
    dones     = dones.to(device_);
    oldLogP   = oldLogP.to(device_);
    oldValues = oldValues.to(device_);

    // ── 2. Compute GAE advantages Â_t ───────────────────────────────────────
    //
    // δ_t  = R_t + γ · V(s_{t+1}) · (1 - done) - V(s_t)
    // Â_t  = Σ_{l=0}^{T-t-1} (γλ)^l · δ_{t+l}
    //
    // This is computed in reverse for numerical efficiency.
    auto advantages = computeGAE(rewards, oldValues, dones);
    auto returns    = advantages + oldValues; // V_target = Â_t + V(s_t)

    // Normalise advantages (zero-mean, unit-variance) for training stability.
    // Guard against single-element batches where std() is undefined.
    if (N > 1) {
        advantages = (advantages - advantages.mean()) /
                     (advantages.std() + 1e-8f);
    }

    // ── 3. PPO optimisation loop ────────────────────────────────────────────
    float totalPolicyLoss = 0.0f, totalValueLoss = 0.0f, totalEntropy = 0.0f;
    int   updateCount     = 0;

    net_->train();

    for (int epoch = 0; epoch < cfg_.ppoEpochs; ++epoch) {
        // Shuffle indices for mini-batch sampling
        auto indices = torch::randperm(N, torch::kLong).to(device_);
        for (int start = 0; start < N; start += cfg_.miniBatchSize) {
            int end    = std::min(start + cfg_.miniBatchSize, N);
            auto mbIdx = indices.slice(0, start, end);

            auto mbStates = states.index_select(0, mbIdx);
            auto mbActs   = actions.index_select(0, mbIdx);
            auto mbAdv    = advantages.index_select(0, mbIdx);
            auto mbRet    = returns.index_select(0, mbIdx);
            auto mbOldLP  = oldLogP.index_select(0, mbIdx);

            // Forward pass through the current policy
            auto [logits, values] = net_->forward(mbStates);
            auto logProbs = torch::log_softmax(logits, /*dim=*/1);
            auto probs    = torch::softmax(logits, /*dim=*/1);

            // log π_θ(a_t | s_t) for the collected actions
            auto newLogP = logProbs.gather(1, mbActs.unsqueeze(1)).squeeze(1);

            // ── Importance ratio r_t(θ) = exp(log π_new - log π_old) ────────
            auto ratio = torch::exp(newLogP - mbOldLP);

            // ── Clipped surrogate objective ──────────────────────────────────
            //   L^{CLIP} = min( r * Â, clip(r, 1-ε, 1+ε) * Â )
            auto surr1  = ratio * mbAdv;
            auto surr2  = torch::clamp(ratio,
                              1.0f - cfg_.epsilon,
                              1.0f + cfg_.epsilon) * mbAdv;
            auto policyLoss = -torch::min(surr1, surr2).mean();

            // ── Value loss (MSE) ─────────────────────────────────────────────
            auto valueLoss = torch::mse_loss(values.squeeze(1), mbRet);

            // ── Entropy bonus S[π_θ] = -Σ π log π ──────────────────────────
            auto entropy = -(probs * logProbs).sum(1).mean();

            // ── Total loss: -L^{CLIP} + c₁·L^{VF} - c₂·S ───────────────────
            auto loss = policyLoss
                      + cfg_.valueLossCoeff * valueLoss
                      - cfg_.entropyCoeff  * entropy;

            // ── Backpropagation + gradient clipping + AdamW step ─────────────
            optimizer_->zero_grad();
            loss.backward();

            // Global gradient norm clipping (prevent exploding gradients)
            torch::nn::utils::clip_grad_norm_(
                net_->parameters(), cfg_.maxGradNorm);

            optimizer_->step();

            totalPolicyLoss += policyLoss.item<float>();
            totalValueLoss  += valueLoss.item<float>();
            totalEntropy    += entropy.item<float>();
            ++updateCount;
        }
    }

    if (updateCount > 0) {
        totalPolicyLoss /= updateCount;
        totalValueLoss  /= updateCount;
        totalEntropy    /= updateCount;
    }

    Logger::get()->debug("[RLAgent] train_step  policy_loss={:.4f}  value_loss={:.4f}  entropy={:.4f}",
        totalPolicyLoss, totalValueLoss, totalEntropy);

    return {totalPolicyLoss, totalValueLoss, totalEntropy};
}

// ── Generalized Advantage Estimation (GAE-λ) ────────────────────────────────
//
// δ_t   = R_t + γ · V(s_{t+1}) · (1 - done_t) − V(s_t)
// Â_t   = δ_t + (γλ)·δ_{t+1} + (γλ)²·δ_{t+2} + ...
//
// Computed via backward scan:
//   Â_{T-1}  = δ_{T-1}
//   Â_t      = δ_t + γλ·(1 - done_t)·Â_{t+1}
// ─────────────────────────────────────────────────────────────────────────────

at::Tensor RLTradingAgent::computeGAE(
        const at::Tensor& rewards,
        const at::Tensor& values,
        const at::Tensor& dones) const {
    const int64_t T = rewards.size(0);
    auto advantages = torch::zeros_like(rewards);
    float gae = 0.0f;

    for (int64_t t = T - 1; t >= 0; --t) {
        // V(s_{t+1}): for the last step we treat the next value as 0
        float nextVal = (t + 1 < T) ? values[t + 1].item<float>() : 0.0f;
        float notDone = 1.0f - dones[t].item<float>();

        // TD residual: δ_t = R_t + γ · V(s_{t+1}) · (1-done) - V(s_t)
        float delta = rewards[t].item<float>()
                    + cfg_.gamma * nextVal * notDone
                    - values[t].item<float>();

        // GAE recursive accumulation:
        //   Â_t = δ_t + γλ · (1-done) · Â_{t+1}
        gae = delta + cfg_.gamma * cfg_.lambda * notDone * gae;
        advantages[t] = gae;
    }
    return advantages;
}

// ── Serialisation ────────────────────────────────────────────────────────────

bool RLTradingAgent::save(const std::string& path) const {
    if (!ready_) return false;
    try {
        torch::save(net_, path);
        Logger::get()->info("[RLAgent] Model saved to {}", path);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->warn("[RLAgent] save failed: {}", e.what());
        return false;
    }
}

bool RLTradingAgent::load(const std::string& path) {
    try {
        torch::load(net_, path, device_);
        // Rebuild optimizer for the (potentially new) parameters
        optimizer_ = std::make_unique<torch::optim::AdamW>(
            net_->parameters(),
            torch::optim::AdamWOptions(cfg_.learningRate)
                .weight_decay(cfg_.weightDecay));
        ready_ = true;
        Logger::get()->info("[RLAgent] Model loaded from {}", path);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->warn("[RLAgent] load failed: {}", e.what());
        return false;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Stub implementation (LibTorch not available)
// ═════════════════════════════════════════════════════════════════════════════
#else

RLTradingAgent::RLTradingAgent(const PPOConfig& cfg) : cfg_(cfg) {
    Logger::get()->warn("[RLAgent] LibTorch not available — using stub");
}

RLTradingAgent::~RLTradingAgent() = default;

RLTradingAgent::ActionResult
RLTradingAgent::forward_pass(const std::vector<float>& /*stateVec*/) const {
    // Default: HOLD with zero log-prob and zero value
    return {1, 0.0f, 0.0f};
}

RLTradingAgent::TrainResult
RLTradingAgent::train_step(const ExperienceBatch& /*batch*/) {
    return {0.0f, 0.0f, 0.0f};
}

bool RLTradingAgent::save(const std::string&) const { return false; }
bool RLTradingAgent::load(const std::string&) { return false; }

#endif // USE_LIBTORCH

}} // namespace crypto::ai
