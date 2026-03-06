#pragma once
#include "../exchange/IExchange.h"
#include "../strategy/RiskManager.h"
#include <string>
#include <vector>
#include <mutex>

namespace crypto {

// Extended order request for UI
struct UIOrderRequest {
    std::string symbol;
    std::string side;       // "BUY" / "SELL"
    std::string type;       // "MARKET" / "LIMIT" / "STOP_LIMIT"
    double      quantity{0.0};
    double      price{0.0};      // for LIMIT / STOP_LIMIT
    double      stopPrice{0.0};  // for STOP_LIMIT
    bool        reduceOnly{false}; // futures only
};

struct PositionModify {
    double newTP{0.0};   // Take Profit price
    double newSL{0.0};   // Stop Loss price
};

struct ManagedPosition {
    std::string symbol;
    std::string side;       // "BUY" / "SELL" 
    double      quantity{0.0};
    double      entryPrice{0.0};
    double      currentPnl{0.0};
    double      tp{0.0};        // Take Profit
    double      sl{0.0};        // Stop Loss
};

// Validates orders against risk rules, formats for exchange APIs
class OrderManagement {
public:
    struct ValidationResult {
        bool   valid{false};
        std::string reason;
    };

    // Validate order against risk rules
    static ValidationResult validate(const UIOrderRequest& req,
                                      const RiskManager& risk,
                                      double accountBalance);

    // Convert UIOrderRequest to exchange OrderRequest
    static OrderRequest toExchangeOrder(const UIOrderRequest& req);

    // Create a close-position market order (opposite side)
    static UIOrderRequest createCloseOrder(const ManagedPosition& pos);

    // Estimate order cost (including commission)
    static double estimateCost(double qty, double price);

    // Adaptive price formatting: fewer decimals for large prices, more for small
    // Returns format string like "%.2f", "%.4f", "%.8f"
    static const char* priceFormat(double price);
    // Format a price value into a buffer with adaptive decimals
    static void formatPrice(char* buf, size_t bufSize, double price);
};

} // namespace crypto
