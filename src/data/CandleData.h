#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace crypto {

struct Candle {
    int64_t openTime{0};
    int64_t closeTime{0};
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    double volume{0.0};
    int64_t trades{0};
    bool closed{false};  // kline.x — bar is closed
};

struct OrderBook {
    struct Level {
        double price{0.0};
        double qty{0.0};
    };
    std::vector<Level> bids;
    std::vector<Level> asks;
    int64_t lastUpdateId{0};
};

struct Trade {
    int64_t id{0};
    double price{0.0};
    double qty{0.0};
    int64_t time{0};
    bool isBuyerMaker{false};
};

struct Signal {
    enum class Type { BUY, SELL, HOLD };
    Type type{Type::HOLD};
    double price{0.0};
    double confidence{0.0};
    double stopLoss{0.0};
    double takeProfit{0.0};
    std::string reason;
};

struct Position {
    enum class Side { LONG, SHORT, NONE };
    Side side{Side::NONE};
    double entryPrice{0.0};
    double qty{0.0};
    double stopLoss{0.0};
    double takeProfit{0.0};
    double trailingStop{0.0};
    int64_t openTime{0};
};

} // namespace crypto
