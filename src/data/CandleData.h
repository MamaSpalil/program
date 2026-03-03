#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

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

// Individual trade tick (real-time market data)
struct Tick {
    long long time{0};   // Unix ms
    double price{0.0};
    double qty{0.0};
    bool isSell{false};  // true = seller is maker
};

// Aggregated price bar (OHLCV)
struct Bar {
    long long openTime{0};
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    double volume{0.0};
    bool isNew{true};
    bool isClosed{false};
};

// Parse a REST kline JSON response into Bar objects.
// Expected JSON array format: [[openTime, open, high, low, close, volume, ...], ...]
// Works with Binance, OKX, and similar exchange kline responses.
inline std::vector<Bar> parseBars(const std::string& json) {
    auto j = nlohmann::json::parse(json);
    std::vector<Bar> bars;
    bars.reserve(j.size());
    for (auto& k : j) {
        Bar b;
        b.openTime = k[0].get<long long>();
        b.open     = std::stod(k[1].get<std::string>());
        b.high     = std::stod(k[2].get<std::string>());
        b.low      = std::stod(k[3].get<std::string>());
        b.close    = std::stod(k[4].get<std::string>());
        b.volume   = std::stod(k[5].get<std::string>());
        b.isNew    = false;
        b.isClosed = true;
        bars.push_back(b);
    }
    return bars;
}

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
