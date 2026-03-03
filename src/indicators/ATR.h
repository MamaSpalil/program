#pragma once
#include "IIndicator.h"
#include <deque>

namespace crypto {

class ATR : public IIndicator {
public:
    explicit ATR(int period);

    void update(const Candle& c) override;
    double value() const override;
    bool ready() const override;

private:
    int period_;
    double atr_{0.0};
    double prevClose_{-1.0};
    int count_{0};
    double sumTR_{0.0};
};

} // namespace crypto
