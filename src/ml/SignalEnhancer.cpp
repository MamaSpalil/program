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

    // Weighted ensemble
    std::vector<double> ensemble(3);
    for (int i = 0; i < 3; ++i) {
        ensemble[i] = weightInd_  * pInd[i]
                    + weightLstm_ * pLstm[i]
                    + weightXgb_  * pXgb[i];
    }

    // Winning class
    int winner = 0;
    for (int i = 1; i < 3; ++i)
        if (ensemble[i] > ensemble[winner]) winner = i;

    outConfidence = ensemble[winner];
    return outConfidence;
}

void SignalEnhancer::recordOutcome(int predicted, int actual) {
    outcomes_.push_back({predicted, actual});
    if (static_cast<int>(outcomes_.size()) > cfg_.rollingWindow) {
        outcomes_.pop_front();
    }
    updateWeights();
}

void SignalEnhancer::updateWeights() {
    // Simple: keep fixed weights for now; online update can be added
    // In production, compare per-model accuracy and rebalance
}

} // namespace crypto
