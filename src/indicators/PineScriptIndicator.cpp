#include "PineScriptIndicator.h"
#include <cmath>
#include <numeric>

namespace crypto {

PineScriptIndicator::PineScriptIndicator(const Config& cfg)
    : cfg_(cfg),
      emaFast_(cfg.emaFast), emaSlow_(cfg.emaSlow), emaTrend_(cfg.emaTrend),
      macdFastEma_(cfg.macdFast), macdSlowEma_(cfg.macdSlow),
      macdSignalEma_(cfg.macdSignal),
      rsi_(cfg.rsiPeriod), atr_(cfg.atrPeriod) {}

void PineScriptIndicator::update(const Candle& c) {
    prevEmaFast_ = emaFast_.ready() ? emaFast_.value() : 0.0;
    prevEmaSlow_ = emaSlow_.ready() ? emaSlow_.value() : 0.0;

    emaFast_.update(c);
    emaSlow_.update(c);
    emaTrend_.update(c);
    rsi_.update(c);
    atr_.update(c);

    macdFastEma_.updateValue(c.close);
    macdSlowEma_.updateValue(c.close);
    updateMACD();

    closes_.push_back(c.close);
    if (static_cast<int>(closes_.size()) > cfg_.bbPeriod * 3) {
        closes_.pop_front();
    }
    updateBB();
}

void PineScriptIndicator::updateMACD() {
    if (!macdFastEma_.ready() || !macdSlowEma_.ready()) return;
    macd_.macd = macdFastEma_.value() - macdSlowEma_.value();
    macdSignalEma_.updateValue(macd_.macd);
    if (macdSignalEma_.ready()) {
        macd_.signal = macdSignalEma_.value();
        macd_.histogram = macd_.macd - macd_.signal;
    }
}

void PineScriptIndicator::updateBB() {
    int n = static_cast<int>(closes_.size());
    if (n < cfg_.bbPeriod) return;

    double sum = 0.0;
    for (int i = n - cfg_.bbPeriod; i < n; ++i) sum += closes_[i];
    double mean = sum / cfg_.bbPeriod;

    double var = 0.0;
    for (int i = n - cfg_.bbPeriod; i < n; ++i) {
        double d = closes_[i] - mean;
        var += d * d;
    }
    double stddev = std::sqrt(var / cfg_.bbPeriod);

    bb_.middle = mean;
    bb_.upper  = mean + cfg_.bbStddev * stddev;
    bb_.lower  = mean - cfg_.bbStddev * stddev;
}

bool PineScriptIndicator::ready() const {
    return emaFast_.ready() && emaSlow_.ready() && emaTrend_.ready() &&
           rsi_.ready() && atr_.ready() && macdSignalEma_.ready();
}

Signal PineScriptIndicator::evaluate(double mlConfidence, double mlThreshold) const {
    Signal sig;
    sig.confidence = mlConfidence;

    if (!ready()) {
        sig.type = Signal::Type::HOLD;
        return sig;
    }

    double ef = emaFast_.value(), es = emaSlow_.value(), et = emaTrend_.value();
    double rsi = rsi_.value();
    bool macdPos = macd_.histogram > 0.0;
    bool macdNeg = macd_.histogram < 0.0;
    bool mlOk = mlConfidence >= mlThreshold;

    // Long condition
    bool longCond = ta::crossover(ef, prevEmaFast_, es, prevEmaSlow_) &&
                    rsi < cfg_.rsiOverbought &&
                    macdPos &&
                    closes_.back() > et &&
                    mlOk;

    // Short condition
    bool shortCond = ta::crossunder(ef, prevEmaFast_, es, prevEmaSlow_) &&
                     rsi > cfg_.rsiOversold &&
                     macdNeg &&
                     closes_.back() < et &&
                     mlOk;

    double atr = atr_.value();
    double price = closes_.empty() ? 0.0 : closes_.back();

    if (longCond) {
        sig.type = Signal::Type::BUY;
        sig.price = price;
        sig.stopLoss = price - 1.5 * atr;
        sig.takeProfit = price + 3.0 * atr;
        sig.reason = "EMA cross up + RSI + MACD + trend + ML";
    } else if (shortCond) {
        sig.type = Signal::Type::SELL;
        sig.price = price;
        sig.stopLoss = price + 1.5 * atr;
        sig.takeProfit = price - 3.0 * atr;
        sig.reason = "EMA cross down + RSI + MACD + trend + ML";
    } else {
        sig.type = Signal::Type::HOLD;
    }
    return sig;
}

} // namespace crypto
