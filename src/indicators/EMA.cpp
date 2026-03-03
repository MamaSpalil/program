#include "EMA.h"

namespace crypto {

EMA::EMA(int period)
    : period_(period), k_(2.0 / (period + 1)) {}

void EMA::update(const Candle& c) {
    updateValue(c.close);
}

void EMA::updateValue(double val) {
    ++count_;
    if (count_ <= period_) {
        sumInit_ += val;
        if (count_ == period_) {
            ema_ = sumInit_ / period_;
        }
    } else {
        ema_ = val * k_ + ema_ * (1.0 - k_);
    }
}

double EMA::value() const {
    return ema_;
}

bool EMA::ready() const {
    return count_ >= period_;
}

} // namespace crypto
