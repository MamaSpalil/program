#include "RSI.h"
#include <algorithm>

namespace crypto {

RSI::RSI(int period) : period_(period) {}

void RSI::update(const Candle& c) {
    if (prevClose_ < 0.0) {
        prevClose_ = c.close;
        return;
    }

    double change = c.close - prevClose_;
    double gain = std::max(change, 0.0);
    double loss = std::max(-change, 0.0);
    prevClose_ = c.close;
    ++count_;

    if (count_ <= period_) {
        sumGain_ += gain;
        sumLoss_ += loss;
        if (count_ == period_) {
            avgGain_ = sumGain_ / period_;
            avgLoss_ = sumLoss_ / period_;
            if (avgLoss_ == 0.0) {
                rsi_ = 100.0;
            } else {
                double rs = avgGain_ / avgLoss_;
                rsi_ = 100.0 - 100.0 / (1.0 + rs);
            }
        }
    } else {
        avgGain_ = (avgGain_ * (period_ - 1) + gain) / period_;
        avgLoss_ = (avgLoss_ * (period_ - 1) + loss) / period_;
        if (avgLoss_ == 0.0) {
            rsi_ = 100.0;
        } else {
            double rs = avgGain_ / avgLoss_;
            rsi_ = 100.0 - 100.0 / (1.0 + rs);
        }
    }
}

double RSI::value() const {
    return rsi_;
}

bool RSI::ready() const {
    return count_ >= period_;
}

} // namespace crypto
