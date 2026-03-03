#include "ModelTrainer.h"
#include "../core/Logger.h"
#include <chrono>
#include <cmath>

namespace crypto {

ModelTrainer::ModelTrainer(LSTMModel& lstm, XGBoostModel& xgb,
                            FeatureExtractor& feat, const Config& cfg)
    : lstm_(lstm), xgb_(xgb), feat_(feat), cfg_(cfg),
      lastRetrainTime_(std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch()).count()) {}

void ModelTrainer::addCandle(const Candle& c) {
    candleHistory_.push_back(c);
    size_t maxCandles = cfg_.lookbackDays * 96UL;  // 96 × 15-min candles/day
    if (candleHistory_.size() > maxCandles) candleHistory_.pop_front();

    feat_.update(c);
    auto f = feat_.extract();
    if (f.valid) {
        featureHistory_.push_back(f.values);
        if (featureHistory_.size() > maxCandles) featureHistory_.pop_front();
    }
}

bool ModelTrainer::shouldRetrain() const {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return (now - lastRetrainTime_) >= cfg_.retrainIntervalHours * 3600LL;
}

void ModelTrainer::buildDataset(std::vector<std::vector<double>>& X,
                                 std::vector<int>& y) const {
    int N = static_cast<int>(featureHistory_.size());
    int future = cfg_.futureCandles;
    if (N <= future) return;

    for (int i = 0; i < N - future; ++i) {
        if (i + future >= static_cast<int>(candleHistory_.size())) break;
        double curClose  = candleHistory_[i].close;
        double futClose  = candleHistory_[i + future].close;
        if (curClose <= 0.0) continue;
        double ret = futClose / curClose - 1.0;
        int label;
        if (ret > cfg_.returnThreshold) label = 0;        // BUY
        else if (ret < -cfg_.returnThreshold) label = 2;  // SELL
        else label = 1;                                     // HOLD

        X.push_back(featureHistory_[i]);
        y.push_back(label);
    }
}

void ModelTrainer::retrain() {
    std::vector<std::vector<double>> X;
    std::vector<int> y;
    buildDataset(X, y);
    if (X.empty()) {
        Logger::get()->debug("ModelTrainer: not enough data to retrain");
        lastRetrainTime_ = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return;
    }
    Logger::get()->info("Retraining on {} samples", X.size());
    xgb_.train(X, y);
    lstm_.train(X, y, cfg_.lstmEpochs, cfg_.lstmLr);

    lastRetrainTime_ = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace crypto
