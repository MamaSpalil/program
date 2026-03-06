#include "OrderManagement.h"
#include "../core/Logger.h"
#include <cstdio>
#include <cmath>

namespace crypto {

OrderManagement::ValidationResult OrderManagement::validate(
    const UIOrderRequest& req,
    const RiskManager& risk,
    double accountBalance)
{
    ValidationResult result;

    if (req.symbol.empty()) {
        result.reason = "Symbol is required";
        return result;
    }
    if (req.quantity <= 0.0) {
        result.reason = "Quantity must be positive";
        return result;
    }
    if (req.type == "LIMIT" || req.type == "STOP_LIMIT") {
        if (req.price <= 0.0) {
            result.reason = "Price must be positive for LIMIT orders";
            return result;
        }
    }
    if (req.type == "STOP_LIMIT" && req.stopPrice <= 0.0) {
        result.reason = "Stop price required for STOP_LIMIT orders";
        return result;
    }

    // Check risk manager
    if (!risk.canTrade()) {
        result.reason = "Risk limit reached (max drawdown or max positions)";
        return result;
    }

    // Check estimated cost vs balance
    double cost = estimateCost(req.quantity, req.price > 0 ? req.price : 0.0);
    if (cost > 0 && cost > accountBalance) {
        result.reason = "Insufficient balance ($" + std::to_string(accountBalance) + ")";
        return result;
    }

    result.valid = true;
    return result;
}

OrderRequest OrderManagement::toExchangeOrder(const UIOrderRequest& req) {
    OrderRequest er;
    er.symbol = req.symbol;
    er.side = (req.side == "BUY") ? OrderRequest::Side::BUY : OrderRequest::Side::SELL;
    if (req.type == "MARKET")
        er.type = OrderRequest::Type::MARKET;
    else if (req.type == "LIMIT")
        er.type = OrderRequest::Type::LIMIT;
    else if (req.type == "STOP_LIMIT")
        er.type = OrderRequest::Type::STOP_LOSS;
    else
        er.type = OrderRequest::Type::MARKET;
    er.qty = req.quantity;
    er.price = req.price;
    er.stopPrice = req.stopPrice;
    return er;
}

UIOrderRequest OrderManagement::createCloseOrder(const ManagedPosition& pos) {
    UIOrderRequest req;
    req.symbol = pos.symbol;
    req.side = (pos.side == "BUY") ? "SELL" : "BUY"; // opposite
    req.type = "MARKET";
    req.quantity = std::abs(pos.quantity);
    req.reduceOnly = true;
    return req;
}

double OrderManagement::estimateCost(double qty, double price) {
    if (price <= 0.0) return 0.0;
    // Include estimated taker commission (0.1% default for most exchanges)
    constexpr double defaultCommissionRate = 0.001;
    double notional = qty * price;
    return notional + notional * defaultCommissionRate;
}

const char* OrderManagement::priceFormat(double price) {
    double abs_price = std::fabs(price);
    if (abs_price >= 1000.0) return "%.2f";
    if (abs_price >= 1.0)    return "%.4f";
    if (abs_price >= 0.01)   return "%.6f";
    return "%.8f";
}

void OrderManagement::formatPrice(char* buf, size_t bufSize, double price) {
    snprintf(buf, bufSize, priceFormat(price), price);
}

} // namespace crypto
