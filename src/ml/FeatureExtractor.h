#pragma once
#include "../data/CandleData.h"
#include "../indicators/IndicatorEngine.h"
#include <vector>
#include <deque>

namespace crypto {

class FeatureExtractor {
public:
    static constexpr int FEATURE_DIM = 60;

    struct Features {
        std::vector<double> values;
        bool valid{false};
    };

    explicit FeatureExtractor(const IndicatorEngine& ind);

    void update(const Candle& c);
    Features extract() const;

private:
    const IndicatorEngine& ind_;
    std::deque<Candle> history_;
    std::deque<double> obv_;

    static constexpr size_t MAX_HISTORY = 250;

    double logReturn(size_t i, size_t j) const;
    double rollingStd(size_t end, int n) const;
    double vwap(int n) const;
};

} // namespace crypto
