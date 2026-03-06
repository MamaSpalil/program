#pragma once
#include <string>

namespace crypto {

/// Utility class for converting between exchange-specific symbol formats.
/// Unified format: "BTCUSDT" (uppercase, no separators).
///
/// Symbol format per exchange:
///   | Exchange | Spot       | Futures          |
///   |----------|------------|------------------|
///   | Binance  | BTCUSDT    | BTCUSDT          |
///   | Bybit    | BTCUSDT    | BTCUSDT          |
///   | OKX      | BTC-USDT   | BTC-USDT-SWAP    |
///   | KuCoin   | BTC-USDT   | BTC-USDT         |
///   | Bitget   | BTCUSDT    | BTCUSDT_UMCBL    |
class SymbolFormatter {
public:
    /// Convert unified base+quote to exchange-specific spot symbol.
    /// @param exchange  Exchange name (case-insensitive): "binance","bybit","okx","kucoin","bitget"
    /// @param base      Base asset, e.g. "BTC"
    /// @param quote     Quote asset, e.g. "USDT"
    static std::string toSpot(const std::string& exchange,
                              const std::string& base,
                              const std::string& quote);

    /// Convert unified base+quote to exchange-specific futures symbol.
    static std::string toFutures(const std::string& exchange,
                                 const std::string& base,
                                 const std::string& quote);

    /// Convert exchange-specific symbol to unified format "BTCUSDT".
    /// Strips separators (-), suffixes (_UMCBL, -SWAP).
    static std::string toUnified(const std::string& exchange,
                                 const std::string& rawSymbol);

    /// Extract base asset from unified symbol (e.g. "BTCUSDT" → "BTC").
    /// Assumes quote is USDT/USDC/BUSD. Returns empty if not recognized.
    static std::string extractBase(const std::string& unified);

    /// Extract quote asset from unified symbol (e.g. "BTCUSDT" → "USDT").
    static std::string extractQuote(const std::string& unified);

private:
    static std::string toUpper(const std::string& s);
    static std::string toLower(const std::string& s);
};

} // namespace crypto
