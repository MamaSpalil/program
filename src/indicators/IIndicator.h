#pragma once
#include "../data/CandleData.h"
#include <deque>

namespace crypto {

// Interface for all technical indicators
class IIndicator {
public:
    virtual ~IIndicator() = default;
    virtual void update(const Candle& c) = 0;
    virtual double value() const = 0;
    virtual bool ready() const = 0;
};

} // namespace crypto
