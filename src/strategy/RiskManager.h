#pragma once
#include "../data/CandleData.h"
#include <nlohmann/json.hpp>

namespace crypto {

class RiskManager {
public:
    struct Config {
        double maxRiskPerTrade{0.02};
        double maxDrawdown{0.15};
        double atrStopMultiplier{1.5};
        double rrRatio{2.0};
        int maxOpenPositions{3};
    };

    explicit RiskManager(const Config& cfg, double initialCapital);

    // Position sizing via fixed-fraction
    double positionSize(double entryPrice, double stopLoss) const;

    // Kelly Criterion position size
    double kellySizing(double winRate, double avgWin, double avgLoss) const;

    // Validate if new trade is allowed
    bool canTrade() const;

    // Calculate trailing stop
    // priceSinceEntry: for LONG, pass highest price since entry;
    //                  for SHORT, pass lowest price since entry.
    double trailingStop(double entryPrice, double priceSinceEntry,
                         double atr, bool isLong) const;

    void onTradeOpen(const Position& pos);
    void onTradeClose(double pnl);

    double equity() const { return equity_; }
    double drawdown() const;

private:
    Config cfg_;
    double initialCapital_;
    double equity_;
    double peakEquity_;
    int openPositions_{0};
};

} // namespace crypto
