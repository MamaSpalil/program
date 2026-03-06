#include "SignalEnhancer.h"
#include <numeric>
#include <cmath>

namespace crypto {

SignalEnhancer::SignalEnhancer(LSTMModel& lstm, XGBoostModel& xgb, const Config& cfg)
    : lstm_(lstm), xgb_(xgb), cfg_(cfg),
      weightInd_(cfg.weightIndicator),
      weightLstm_(cfg.weightLstm),
      weightXgb_(cfg.weightXgb) {}

double SignalEnhancer::enhance(int indicatorSignal,
                                const std::vector<double>& features,
                                double& outConfidence) {
    // Map indicator: +1=buy(idx 0), -1=sell(idx 2), 0=hold(idx 1)
    auto toProbs = [](int sig) -> std::vector<double> {
        if (sig > 0) return {0.8, 0.1, 0.1};
        if (sig < 0) return {0.1, 0.1, 0.8};
        return {0.1, 0.8, 0.1};
    };

    auto pInd  = toProbs(indicatorSignal);
    auto pLstm = lstm_.ready()  ? lstm_.predict(features)  : std::vector<double>{0.333,0.334,0.333};
    auto pXgb  = xgb_.ready()   ? xgb_.predict(features)   : std::vector<double>{0.333,0.334,0.333};

    // Weighted ensemble (read weights under lock)
    double wInd, wLstm, wXgb;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        wInd  = weightInd_;
        wLstm = weightLstm_;
        wXgb  = weightXgb_;
    }

    std::vector<double> ensemble(3);
    for (int i = 0; i < 3; ++i) {
        ensemble[i] = wInd   * pInd[i]
                    + wLstm  * pLstm[i]
                    + wXgb   * pXgb[i];
    }

    // Winning class
    int winner = 0;
    for (int i = 1; i < 3; ++i)
        if (ensemble[i] > ensemble[winner]) winner = i;

    outConfidence = ensemble[winner];
    return outConfidence;
}

void SignalEnhancer::recordOutcome(int predicted, int actual) {
    std::lock_guard<std::mutex> lock(mtx_);
    outcomes_.push_back({predicted, actual});
    if (static_cast<int>(outcomes_.size()) > cfg_.rollingWindow) {
        outcomes_.pop_front();
    }
    updateWeights();
}

void SignalEnhancer::updateWeights() {
    if (outcomes_.size() < 10) return;  // Need enough data

    // Compute rolling accuracy per model category
    int correct = 0;
    int total = static_cast<int>(outcomes_.size());
    for (auto& [pred, act] : outcomes_) {
        if (pred == act) ++correct;
    }

    double accuracy = static_cast<double>(correct) / total;

    // Adaptive weight adjustment bounds
    constexpr double kMaxIndicatorWeight = 0.55;
    constexpr double kMinIndicatorWeight = 0.20;
    constexpr double kWeightStep         = 0.01;

    if (accuracy > 0.6) {
        // Good performance — slightly favor indicators for stability
        weightInd_  = std::min(kMaxIndicatorWeight, weightInd_  + kWeightStep);
        weightLstm_ = (1.0 - weightInd_) * 0.5;
        weightXgb_  = (1.0 - weightInd_) * 0.5;
    } else if (accuracy < 0.4) {
        // Poor performance — boost ML models for pattern detection
        weightInd_  = std::max(kMinIndicatorWeight, weightInd_  - kWeightStep);
        weightLstm_ = (1.0 - weightInd_) * 0.5;
        weightXgb_  = (1.0 - weightInd_) * 0.5;
    }
    // Else: keep current weights (neutral zone)
}

} // namespace crypto
