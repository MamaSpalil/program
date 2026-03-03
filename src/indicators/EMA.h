#pragma once
#include "IIndicator.h"

namespace crypto {

class EMA : public IIndicator {
public:
    explicit EMA(int period);

    void update(const Candle& c) override;
    void updateValue(double val);
    double value() const override;
    bool ready() const override;

private:
    int period_;
    double k_;      // smoothing factor = 2/(period+1)
    double ema_{0.0};
    int count_{0};
    double sumInit_{0.0};
};

} // namespace crypto
