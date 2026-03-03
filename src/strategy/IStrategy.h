#pragma once
#include "../data/CandleData.h"
#include <optional>

namespace crypto {

// Interface for trading strategies
class IStrategy {
public:
    virtual ~IStrategy() = default;
    virtual void onCandle(const Candle& c) = 0;
    virtual std::optional<Signal> getSignal() = 0;
    virtual void onFill(const Position& pos) = 0;
};

} // namespace crypto
