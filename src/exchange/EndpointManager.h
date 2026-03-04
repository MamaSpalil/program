#pragma once
#include <string>
#include <vector>

namespace crypto {

class ExchangeEndpointManager {
public:
    // Binance alternative endpoints for geo-blocking fallback
    static const std::vector<std::string>& binanceEndpoints() {
        static const std::vector<std::string> endpoints = {
            "https://api.binance.com",
            "https://api1.binance.com",
            "https://api2.binance.com",
            "https://api3.binance.com",
            "https://api4.binance.com"
        };
        return endpoints;
    }

    static const std::vector<std::string>& binanceFuturesEndpoints() {
        static const std::vector<std::string> endpoints = {
            "https://fapi.binance.com",
            "https://fapi1.binance.com",
            "https://fapi2.binance.com"
        };
        return endpoints;
    }

    // Get next working endpoint (round-robin with retry)
    static std::string getNextEndpoint(
        const std::vector<std::string>& endpoints,
        int& currentIdx)
    {
        if (endpoints.empty()) return "";
        currentIdx = (currentIdx + 1) % static_cast<int>(endpoints.size());
        return endpoints[static_cast<size_t>(currentIdx)];
    }

    // Check if an HTTP status code indicates geo-blocking
    static bool isGeoBlocked(int httpStatus) {
        return httpStatus == 403 || httpStatus == 451;
    }
};

} // namespace crypto
