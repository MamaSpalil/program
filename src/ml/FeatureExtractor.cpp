#include "FeatureExtractor.h"
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <numeric>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace crypto {

FeatureExtractor::FeatureExtractor(const IndicatorEngine& ind) : ind_(ind) {}

void FeatureExtractor::update(const Candle& c) {
    history_.push_back(c);
    if (history_.size() > MAX_HISTORY) history_.pop_front();

    double prevObv = obv_.empty() ? 0.0 : obv_.back();
    if (history_.size() >= 2) {
        double prevClose = history_[history_.size() - 2].close;
        if (c.close > prevClose) prevObv += c.volume;
        else if (c.close < prevClose) prevObv -= c.volume;
    }
    obv_.push_back(prevObv);
    if (obv_.size() > MAX_HISTORY) obv_.pop_front();
}

double FeatureExtractor::logReturn(size_t i, size_t j) const {
    if (i >= history_.size() || j >= history_.size()) return 0.0;
    double a = history_[i].close, b = history_[j].close;
    if (b <= 0.0) return 0.0;
    return std::log(a / b);
}

double FeatureExtractor::rollingStd(size_t end, int n) const {
    if (static_cast<int>(end) < n - 1) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < n; ++i) sum += history_[end - i].close;
    double mean = sum / n;
    double var = 0.0;
    for (int i = 0; i < n; ++i) {
        double d = history_[end - i].close - mean;
        var += d * d;
    }
    return std::sqrt(var / n);
}

double FeatureExtractor::vwap(int n) const {
    int sz = static_cast<int>(history_.size());
    if (sz < n) return 0.0;
    double sumPV = 0.0, sumV = 0.0;
    for (int i = sz - n; i < sz; ++i) {
        double typical = (history_[i].high + history_[i].low + history_[i].close) / 3.0;
        sumPV += typical * history_[i].volume;
        sumV  += history_[i].volume;
    }
    return sumV > 0.0 ? sumPV / sumV : 0.0;
}

FeatureExtractor::Features FeatureExtractor::extract() const {
    Features f;
    int sz = static_cast<int>(history_.size());
    if (sz < 21) return f;

    f.values.reserve(FEATURE_DIM);
    int last = sz - 1;

    // Log returns [1,3,5,10,20]
    for (int lag : {1,3,5,10,20}) {
        f.values.push_back(last >= lag ? logReturn(last, last - lag) : 0.0);
    }
    // Volatility [5,10,20]
    for (int n : {5,10,20}) {
        f.values.push_back(last >= n - 1 ? rollingStd(last, n) : 0.0);
    }
    // Price vs EMA
    double price = history_[last].close;
    for (double ema : {ind_.emaFast(), ind_.emaSlow(), ind_.emaTrend()}) {
        f.values.push_back(ema > 0.0 ? (price - ema) / ema : 0.0);
    }
    // high_low_ratio, body_ratio
    const auto& c = history_[last];
    double hl = c.high - c.low;
    f.values.push_back(hl > 0.0 ? hl / c.close : 0.0);
    f.values.push_back(hl > 0.0 ? std::abs(c.open - c.close) / hl : 0.0);

    // Volume ratios
    auto avgVol = [&](int n) {
        double s = 0.0;
        for (int i = std::max(0, sz - n); i < sz; ++i) s += history_[i].volume;
        return s / std::min(n, sz);
    };
    double av5 = avgVol(5), av20 = avgVol(20);
    f.values.push_back(av5  > 0.0 ? c.volume / av5  : 1.0);
    f.values.push_back(av20 > 0.0 ? c.volume / av20 : 1.0);

    // VWAP deviation
    double vw = vwap(20);
    f.values.push_back(vw > 0.0 ? (price - vw) / vw : 0.0);

    // OBV trend (slope over last 5)
    if (static_cast<int>(obv_.size()) >= 5) {
        double obvSlope = (obv_.back() - obv_[obv_.size() - 5]) / 5.0;
        f.values.push_back(obvSlope);
    } else {
        f.values.push_back(0.0);
    }

    // Indicator values
    f.values.push_back(ind_.rsi() / 100.0);
    f.values.push_back(ind_.emaFast() > 0.0 ? (price - ind_.emaFast()) / ind_.emaFast() : 0.0);
    auto macd = ind_.macd();
    f.values.push_back(macd.macd);
    f.values.push_back(macd.signal);
    f.values.push_back(macd.histogram);
    auto bb = ind_.bb();
    double bbRange = bb.upper - bb.lower;
    f.values.push_back(bbRange > 0.0 ? (price - bb.lower) / bbRange : 0.5);
    f.values.push_back(ind_.atr() > 0.0 ? ind_.atr() / price : 0.0);

    // Time features (cyclical)
    // Use openTime from last candle (ms -> seconds -> hours)
    double ts = static_cast<double>(c.openTime) / 1000.0;
    double hour = std::fmod(ts / 3600.0, 24.0);
    double dow  = std::fmod(ts / 86400.0, 7.0);
    f.values.push_back(std::sin(2.0 * M_PI * hour / 24.0));
    f.values.push_back(std::cos(2.0 * M_PI * hour / 24.0));
    f.values.push_back(std::sin(2.0 * M_PI * dow  / 7.0));
    f.values.push_back(std::cos(2.0 * M_PI * dow  / 7.0));

    // Pad to FEATURE_DIM
    while (static_cast<int>(f.values.size()) < FEATURE_DIM) {
        f.values.push_back(0.0);
    }
    f.values.resize(FEATURE_DIM);
    f.valid = true;
    return f;
}

} // namespace crypto
