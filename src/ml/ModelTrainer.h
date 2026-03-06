#pragma once
#include "FeatureExtractor.h"
#include "LSTMModel.h"
#include "XGBoostModel.h"
#include "../data/CandleData.h"
#include <deque>
#include <mutex>
#include <vector>
#include <nlohmann/json.hpp>

namespace crypto {

class ModelTrainer {
public:
    struct Config {
        int lstmEpochs{50};
        double lstmLr{0.001};
        int xgbRounds{500};
        int lookbackDays{90};
        int futureCandles{5};
        double returnThreshold{0.003};  // 0.3%
        int retrainIntervalHours{24};
        int timeframeMinutes{15};       // candle interval in minutes (default 15m)
    };

    ModelTrainer(LSTMModel& lstm, XGBoostModel& xgb,
                 FeatureExtractor& feat, const Config& cfg);

    void addCandle(const Candle& c);
    bool shouldRetrain() const;
    void retrain();

    /// Get last retrain time (epoch seconds); 0 if never retrained
    int64_t getLastRetrainTime() const { return lastRetrainTime_; }

private:
    LSTMModel& lstm_;
    XGBoostModel& xgb_;
    FeatureExtractor& feat_;
    Config cfg_;

    std::deque<Candle> candleHistory_;
    std::deque<std::vector<double>> featureHistory_;
    int64_t lastRetrainTime_{0};

    void buildDataset(std::vector<std::vector<double>>& X,
                      std::vector<int>& y) const;
};

} // namespace crypto
