#pragma once
#include "IStrategy.h"
#include "RiskManager.h"
#include "../indicators/IndicatorEngine.h"
#include "../ml/FeatureExtractor.h"
#include "../ml/LSTMModel.h"
#include "../ml/XGBoostModel.h"
#include "../ml/SignalEnhancer.h"
#include "../ml/ModelTrainer.h"
#include <optional>
#include <nlohmann/json.hpp>

namespace crypto {

class MLEnhancedStrategy : public IStrategy {
public:
    MLEnhancedStrategy(const nlohmann::json& config, double initialCapital);

    void onCandle(const Candle& c) override;
    std::optional<Signal> getSignal() override;
    void onFill(const Position& pos) override;

    const IndicatorEngine& indicators() const { return indEngine_; }
    const RiskManager& riskManager() const { return riskMgr_; }

private:
    IndicatorEngine indEngine_;
    LSTMConfig lstmCfg_;
    LSTMModel lstmModel_;
    XGBoostConfig xgbCfg_;
    XGBoostModel xgbModel_;
    RiskManager riskMgr_;
    FeatureExtractor featExtract_;
    SignalEnhancer enhancer_;
    ModelTrainer trainer_;

    std::optional<Signal> lastSignal_;
    std::optional<Position> openPos_;
    double minConfidence_{0.65};

    void checkEarlyExit(const Candle& c);
};

} // namespace crypto
