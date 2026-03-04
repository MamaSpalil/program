#pragma once
#include <vector>
#include <cmath>
#include <numeric>
#include "../data/CandleData.h"

namespace crypto {

// VWAP (Volume-Weighted Average Price)
inline std::vector<double> calcVWAP(const std::vector<Bar>& bars) {
    std::vector<double> vwap;
    vwap.reserve(bars.size());
    double cumTPV = 0.0;  // cumulative typical_price * volume
    double cumVol = 0.0;
    for (auto& b : bars) {
        double tp = (b.high + b.low + b.close) / 3.0;
        cumTPV += tp * b.volume;
        cumVol += b.volume;
        vwap.push_back(cumVol > 0.0 ? cumTPV / cumVol : tp);
    }
    return vwap;
}

// TWAP (Time-Weighted Average Price)
inline std::vector<double> calcTWAP(const std::vector<Bar>& bars) {
    std::vector<double> twap;
    twap.reserve(bars.size());
    double cumPrice = 0.0;
    for (size_t i = 0; i < bars.size(); ++i) {
        double tp = (bars[i].high + bars[i].low + bars[i].close) / 3.0;
        cumPrice += tp;
        twap.push_back(cumPrice / static_cast<double>(i + 1));
    }
    return twap;
}

// Pearson correlation coefficient
inline double pearson(const std::vector<double>& x,
                      const std::vector<double>& y) {
    if (x.size() != y.size() || x.empty()) return 0.0;
    size_t n = x.size();
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;
    for (size_t i = 0; i < n; ++i) {
        sumX  += x[i];
        sumY  += y[i];
        sumXY += x[i] * y[i];
        sumX2 += x[i] * x[i];
        sumY2 += y[i] * y[i];
    }
    double denom = std::sqrt(
        (static_cast<double>(n) * sumX2 - sumX * sumX) *
        (static_cast<double>(n) * sumY2 - sumY * sumY));
    if (denom < 1e-15) return 0.0;
    return (static_cast<double>(n) * sumXY - sumX * sumY) / denom;
}

} // namespace crypto
