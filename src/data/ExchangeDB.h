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
    bool testnet{false};
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

private:
    std::string dbPath_;
    std::vector<ExchangeProfile> profiles_;
    std::string activeProfile_;
    mutable std::mutex mutex_;
};

} // namespace crypto
