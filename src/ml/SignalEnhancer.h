#pragma once
#include "LSTMModel.h"
#include "XGBoostModel.h"
#include "FeatureExtractor.h"
#include "../data/CandleData.h"
#include <deque>

namespace crypto {

class SignalEnhancer {
public:
    struct Config {
        double weightIndicator{0.4};
        double weightLstm{0.3};
        double weightXgb{0.3};
        double minConfidence{0.65};
        int rollingWindow{50};
    };

    SignalEnhancer(LSTMModel& lstm, XGBoostModel& xgb, const Config& cfg);

    // Returns enhanced confidence for a given indicator signal direction.
    // indicatorSignal: +1 = BUY, -1 = SELL, 0 = HOLD
    // Returns final confidence [0,1] and updates weights based on accuracy
    double enhance(int indicatorSignal,
                   const std::vector<double>& features,
                   double& outConfidence);

    void recordOutcome(int predicted, int actual);

private:
    LSTMModel& lstm_;
    XGBoostModel& xgb_;
    Config cfg_;

    // Rolling accuracy
    std::deque<std::pair<int,int>> outcomes_;

    double weightInd_{0.4};
    double weightLstm_{0.3};
    double weightXgb_{0.3};

    void updateWeights();
};

} // namespace crypto
