#include "RiskManager.h"
#include "../core/Logger.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace crypto {

RiskManager::RiskManager(const Config& cfg, double initialCapital)
    : cfg_(cfg), initialCapital_(initialCapital),
      equity_(initialCapital), peakEquity_(initialCapital) {}

double RiskManager::positionSize(double entryPrice, double stopLoss) const {
    if (entryPrice <= 0.0 || stopLoss <= 0.0) return 0.0;
    double risk = equity_ * cfg_.maxRiskPerTrade;
    double riskPerUnit = std::abs(entryPrice - stopLoss);
    if (riskPerUnit <= 0.0) return 0.0;
    return risk / riskPerUnit;
}

double RiskManager::kellySizing(double winRate, double avgWin, double avgLoss) const {
    if (avgLoss <= 0.0) return 0.0;
    double b = avgWin / avgLoss;
    double kelly = winRate - (1.0 - winRate) / b;
    kelly = std::max(0.0, std::min(kelly, cfg_.maxRiskPerTrade * 2.0));
    return equity_ * kelly;
}

bool RiskManager::canTrade() const {
    if (drawdown() >= cfg_.maxDrawdown) {
        Logger::get()->warn("RiskManager: max drawdown reached, trading halted");
        return false;
    }
    if (openPositions_ >= cfg_.maxOpenPositions) {
        Logger::get()->debug("RiskManager: max open positions reached");
        return false;
    }
    return true;
}

double RiskManager::trailingStop(double entryPrice, double highSince,
                                  double atr, bool isLong) const {
    if (isLong) {
        // Trail below the highest price since entry
        return std::max(entryPrice,
                        highSince - cfg_.atrStopMultiplier * atr);
    } else {
        // Trail above the lowest price since entry (highSince = lowest low for shorts)
        return std::min(entryPrice,
                        highSince + cfg_.atrStopMultiplier * atr);
        // Note: For SHORT positions, highSince should be passed as the lowest
        // price since entry. The stop is placed above that low by ATR*mult.
    }
}

void RiskManager::onTradeOpen(const Position& /*pos*/) {
    ++openPositions_;
}

void RiskManager::onTradeClose(double pnl) {
    if (openPositions_ > 0) --openPositions_;
    equity_ += pnl;
    peakEquity_ = std::max(peakEquity_, equity_);
    Logger::get()->info("Trade closed PnL={:.2f} equity={:.2f}", pnl, equity_);
}

double RiskManager::drawdown() const {
    if (peakEquity_ <= 0.0) return 0.0;
    return (peakEquity_ - equity_) / peakEquity_;
}

} // namespace crypto
