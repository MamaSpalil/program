#include "ATR.h"
#include <algorithm>
#include <cmath>

namespace crypto {

ATR::ATR(int period) : period_(period) {}

void ATR::update(const Candle& c) {
    double tr;
    if (prevClose_ < 0.0) {
        tr = c.high - c.low;
    } else {
        tr = std::max({c.high - c.low,
                       std::abs(c.high - prevClose_),
                       std::abs(c.low  - prevClose_)});
    }
    prevClose_ = c.close;
    ++count_;

    if (count_ <= period_) {
        sumTR_ += tr;
        if (count_ == period_) {
            atr_ = sumTR_ / period_;
        }
    } else {
        atr_ = (atr_ * (period_ - 1) + tr) / period_;
    }
}

double ATR::value() const {
    return atr_;
}

bool ATR::ready() const {
    return count_ >= period_;
}

} // namespace crypto
