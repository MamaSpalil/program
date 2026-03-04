#pragma once
#include <string>
#include <vector>
#include <mutex>

namespace crypto {

struct TradeMarker {
    double      time{0.0};
    double      price{0.0};
    std::string side;       // "BUY" / "SELL"
    bool        isEntry{true}; // true=entry, false=exit
    double      pnl{0.0};      // only for exits
};

struct TPSLLine {
    double tp{0.0};   // Take Profit price (0 = not set)
    double sl{0.0};   // Stop Loss price (0 = not set)
};

class TradeMarkerManager {
public:
    void addMarker(const TradeMarker& marker);
    void clearMarkers();

    std::vector<TradeMarker> getMarkers() const;

    void setTPSL(const std::string& symbol, double tp, double sl);
    void clearTPSL(const std::string& symbol);
    std::vector<TPSLLine> getTPSLLines() const;

private:
    mutable std::mutex mutex_;
    std::vector<TradeMarker> markers_;
    std::vector<std::pair<std::string, TPSLLine>> tpslLines_;
};

} // namespace crypto
