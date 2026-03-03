#pragma once
#include "IIndicator.h"
#include <deque>

namespace crypto {

class RSI : public IIndicator {
public:
    explicit RSI(int period);

    void update(const Candle& c) override;
    double value() const override;
    bool ready() const override;

private:
    int period_;
    double avgGain_{0.0};
    double avgLoss_{0.0};
    double prevClose_{-1.0};
    int count_{0};
    double rsi_{50.0};
    double sumGain_{0.0};
    double sumLoss_{0.0};
};

} // namespace crypto
