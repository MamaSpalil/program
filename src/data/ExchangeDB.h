#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>
#include <mutex>

namespace crypto {

// A single exchange profile (credentials + settings)
struct ExchangeProfile {
    std::string name;           // unique profile name (e.g. "binance-main")
    std::string exchangeType;   // "binance", "bybit", "okx", "bitget", "kucoin"
    std::string apiKey;
    std::string apiSecret;
    std::string passphrase;     // required for OKX, Bitget, KuCoin
    std::string baseUrl;
    std::string wsHost;
    std::string wsPort;
    bool testnet{false};
};

// A single trade record for statistics tracking
struct TradeRecord {
    int64_t     timestamp{0};       // epoch ms when the trade occurred
    std::string symbol;
    std::string side;               // "BUY" or "SELL"
    std::string orderType;          // "MARKET", "LIMIT"
    double      qty{0.0};
    double      price{0.0};
    double      pnl{0.0};          // realised P&L (0 for entries)
    double      confidence{0.0};    // ML signal confidence at the time
    std::string reason;             // signal reason / description
};

// JSON-file-based storage for exchange profiles.
// Allows saving, loading, listing and switching between exchange profiles.
class ExchangeDB {
public:
    explicit ExchangeDB(const std::string& dbPath = "config/exchanges.json");

    // Load all profiles from disk
    bool load();

    // Save all profiles to disk
    bool save() const;

    // Get all profiles
    std::vector<ExchangeProfile> listProfiles() const;

    // Get a profile by name. Returns nullopt if not found.
    std::optional<ExchangeProfile> getProfile(const std::string& name) const;

    // Add or update a profile
    void upsertProfile(const ExchangeProfile& profile);

    // Remove a profile by name
    bool removeProfile(const std::string& name);

    // Get/set the active profile name
    std::string activeProfileName() const;
    void setActiveProfile(const std::string& name);

    // Convenience: get the currently active profile
    std::optional<ExchangeProfile> activeProfile() const;

    // ── Trade statistics ──────────────────────────────────────────────────
    // Add a trade record (persisted on next save())
    void addTrade(const TradeRecord& tr);

    // Get all recorded trades
    std::vector<TradeRecord> listTrades() const;

    // Summary statistics
    int    totalTrades() const;
    int    winCount() const;
    int    lossCount() const;
    double totalPnl() const;
    double winRate() const;

private:
    std::string dbPath_;
    std::vector<ExchangeProfile> profiles_;
    std::string activeProfile_;
    std::vector<TradeRecord> trades_;
    mutable std::mutex mutex_;
};

} // namespace crypto
