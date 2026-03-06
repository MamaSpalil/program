#include "SymbolFormatter.h"
#include <algorithm>
#include <cctype>

namespace crypto {

std::string SymbolFormatter::toUpper(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return r;
}

std::string SymbolFormatter::toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return r;
}

std::string SymbolFormatter::toSpot(const std::string& exchange,
                                     const std::string& base,
                                     const std::string& quote) {
    std::string ex = toLower(exchange);
    std::string b = toUpper(base);
    std::string q = toUpper(quote);

    if (ex == "okx" || ex == "kucoin") {
        return b + "-" + q;           // BTC-USDT
    }
    // binance, bybit, bitget — no separator
    return b + q;                     // BTCUSDT
}

std::string SymbolFormatter::toFutures(const std::string& exchange,
                                        const std::string& base,
                                        const std::string& quote) {
    std::string ex = toLower(exchange);
    std::string b = toUpper(base);
    std::string q = toUpper(quote);

    if (ex == "okx") {
        return b + "-" + q + "-SWAP";        // BTC-USDT-SWAP
    }
    if (ex == "kucoin") {
        return b + "-" + q;                  // BTC-USDT
    }
    if (ex == "bitget") {
        return b + q + "_UMCBL";             // BTCUSDT_UMCBL
    }
    // binance, bybit — same as spot
    return b + q;                            // BTCUSDT
}

std::string SymbolFormatter::toUnified(const std::string& exchange,
                                        const std::string& rawSymbol) {
    std::string s = toUpper(rawSymbol);
    (void)exchange; // exchange hint not needed for stripping

    // Remove known futures suffixes
    {
        const std::string swap = "-SWAP";
        if (s.size() > swap.size() &&
            s.compare(s.size() - swap.size(), swap.size(), swap) == 0) {
            s.erase(s.size() - swap.size());
        }
    }
    {
        const std::string umcbl = "_UMCBL";
        if (s.size() > umcbl.size() &&
            s.compare(s.size() - umcbl.size(), umcbl.size(), umcbl) == 0) {
            s.erase(s.size() - umcbl.size());
        }
    }

    // Remove dashes (OKX / KuCoin format)
    s.erase(std::remove(s.begin(), s.end(), '-'), s.end());

    return s; // "BTCUSDT"
}

std::string SymbolFormatter::extractBase(const std::string& unified) {
    std::string s = toUpper(unified);

    // Check common quote suffixes (longest first)
    static const std::string quotes[] = {"USDT", "USDC", "BUSD", "BTC", "ETH"};
    for (const auto& q : quotes) {
        if (s.size() > q.size() &&
            s.compare(s.size() - q.size(), q.size(), q) == 0) {
            return s.substr(0, s.size() - q.size());
        }
    }
    return {}; // unrecognized
}

std::string SymbolFormatter::extractQuote(const std::string& unified) {
    std::string s = toUpper(unified);

    static const std::string quotes[] = {"USDT", "USDC", "BUSD", "BTC", "ETH"};
    for (const auto& q : quotes) {
        if (s.size() > q.size() &&
            s.compare(s.size() - q.size(), q.size(), q) == 0) {
            return q;
        }
    }
    return {};
}

} // namespace crypto
