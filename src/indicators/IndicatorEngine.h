#pragma once
#include "PineScriptIndicator.h"
#include <nlohmann/json.hpp>

namespace crypto {

class IndicatorEngine {
public:
    explicit IndicatorEngine(const nlohmann::json& indicatorConfig);

    void update(const Candle& c);
    Signal evaluate(double mlConfidence = 0.0) const;
    bool ready() const;

    double emaFast() const;
    double emaSlow() const;
    double emaTrend() const;
    double rsi() const;
    double atr() const;
    MACDResult macd() const;
    BBResult   bb() const;

private:
    PineScriptIndicator ind_;
    double mlThreshold_{0.65};
};

} // namespace crypto
