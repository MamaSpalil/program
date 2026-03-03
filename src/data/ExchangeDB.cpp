#include "ExchangeDB.h"
#include "../core/Logger.h"
#include <fstream>
#include <filesystem>

namespace crypto {

ExchangeDB::ExchangeDB(const std::string& dbPath)
    : dbPath_(dbPath) {}

bool ExchangeDB::load() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!std::filesystem::exists(dbPath_)) {
        Logger::get()->debug("[ExchangeDB] File not found: {}, starting empty", dbPath_);
        return true; // not an error — just no saved profiles yet
    }

    try {
        std::ifstream f(dbPath_);
        if (!f.is_open()) {
            Logger::get()->warn("[ExchangeDB] Cannot open {}", dbPath_);
            return false;
        }
        nlohmann::json j;
        f >> j;

        activeProfile_ = j.value("active", "");

        profiles_.clear();
        if (j.contains("profiles") && j["profiles"].is_array()) {
            for (auto& p : j["profiles"]) {
                ExchangeProfile prof;
                prof.name         = p.value("name", "");
                prof.exchangeType = p.value("exchange_type", "binance");
                prof.apiKey       = p.value("api_key", "");
                prof.apiSecret    = p.value("api_secret", "");
                prof.passphrase   = p.value("passphrase", "");
                prof.baseUrl      = p.value("base_url", "");
                prof.wsHost       = p.value("ws_host", "");
                prof.wsPort       = p.value("ws_port", "");
                prof.testnet      = p.value("testnet", false);
                profiles_.push_back(std::move(prof));
            }
        }

        // Load trade records
        trades_.clear();
        if (j.contains("trades") && j["trades"].is_array()) {
            for (auto& t : j["trades"]) {
                TradeRecord tr;
                tr.timestamp  = t.value("timestamp", int64_t{0});
                tr.symbol     = t.value("symbol", "");
                tr.side       = t.value("side", "");
                tr.orderType  = t.value("order_type", "");
                tr.qty        = t.value("qty", 0.0);
                tr.price      = t.value("price", 0.0);
                tr.pnl        = t.value("pnl", 0.0);
                tr.confidence = t.value("confidence", 0.0);
                tr.reason     = t.value("reason", "");
                trades_.push_back(std::move(tr));
            }
        }

        Logger::get()->info("[ExchangeDB] Loaded {} profile(s), {} trade(s), active='{}'",
                            profiles_.size(), trades_.size(), activeProfile_);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->warn("[ExchangeDB] Load error: {}", e.what());
        return false;
    }
}

bool ExchangeDB::save() const {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        std::filesystem::create_directories(
            std::filesystem::path(dbPath_).parent_path());

        nlohmann::json j;
        j["active"] = activeProfile_;

        nlohmann::json arr = nlohmann::json::array();
        for (auto& p : profiles_) {
            nlohmann::json pj;
            pj["name"]          = p.name;
            pj["exchange_type"] = p.exchangeType;
            pj["api_key"]       = p.apiKey;
            pj["api_secret"]    = p.apiSecret;
            pj["passphrase"]    = p.passphrase;
            pj["base_url"]      = p.baseUrl;
            pj["ws_host"]       = p.wsHost;
            pj["ws_port"]       = p.wsPort;
            pj["testnet"]       = p.testnet;
            arr.push_back(std::move(pj));
        }
        j["profiles"] = std::move(arr);

        // Persist trade records
        nlohmann::json tradeArr = nlohmann::json::array();
        for (auto& t : trades_) {
            nlohmann::json tj;
            tj["timestamp"]  = t.timestamp;
            tj["symbol"]     = t.symbol;
            tj["side"]       = t.side;
            tj["order_type"] = t.orderType;
            tj["qty"]        = t.qty;
            tj["price"]      = t.price;
            tj["pnl"]        = t.pnl;
            tj["confidence"] = t.confidence;
            tj["reason"]     = t.reason;
            tradeArr.push_back(std::move(tj));
        }
        j["trades"] = std::move(tradeArr);

        std::ofstream f(dbPath_);
        if (!f.is_open()) {
            Logger::get()->warn("[ExchangeDB] Cannot write {}", dbPath_);
            return false;
        }
        f << j.dump(2);
        Logger::get()->debug("[ExchangeDB] Saved {} profile(s), {} trade(s) to {}",
                              profiles_.size(), trades_.size(), dbPath_);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->warn("[ExchangeDB] Save error: {}", e.what());
        return false;
    }
}

std::vector<ExchangeProfile> ExchangeDB::listProfiles() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return profiles_;
}

std::optional<ExchangeProfile> ExchangeDB::getProfile(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& p : profiles_) {
        if (p.name == name) return p;
    }
    return std::nullopt;
}

void ExchangeDB::upsertProfile(const ExchangeProfile& profile) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& p : profiles_) {
        if (p.name == profile.name) {
            p = profile;
            Logger::get()->debug("[ExchangeDB] Updated profile '{}'", profile.name);
            return;
        }
    }
    profiles_.push_back(profile);
    Logger::get()->debug("[ExchangeDB] Added new profile '{}'", profile.name);
}

bool ExchangeDB::removeProfile(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(profiles_.begin(), profiles_.end(),
        [&](const ExchangeProfile& p) { return p.name == name; });
    if (it == profiles_.end()) return false;
    profiles_.erase(it, profiles_.end());
    if (activeProfile_ == name) activeProfile_.clear();
    Logger::get()->debug("[ExchangeDB] Removed profile '{}'", name);
    return true;
}

std::string ExchangeDB::activeProfileName() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeProfile_;
}

void ExchangeDB::setActiveProfile(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    activeProfile_ = name;
    Logger::get()->debug("[ExchangeDB] Active profile set to '{}'", name);
}

std::optional<ExchangeProfile> ExchangeDB::activeProfile() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& p : profiles_) {
        if (p.name == activeProfile_) return p;
    }
    return std::nullopt;
}

// ── Trade statistics ──────────────────────────────────────────────────────

void ExchangeDB::addTrade(const TradeRecord& tr) {
    std::lock_guard<std::mutex> lock(mutex_);
    trades_.push_back(tr);
}

std::vector<TradeRecord> ExchangeDB::listTrades() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return trades_;
}

int ExchangeDB::totalTrades() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(trades_.size());
}

int ExchangeDB::winCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int w = 0;
    for (auto& t : trades_) if (t.pnl > 0.0) ++w;
    return w;
}

int ExchangeDB::lossCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int l = 0;
    for (auto& t : trades_) if (t.pnl < 0.0) ++l;
    return l;
}

double ExchangeDB::totalPnl() const {
    std::lock_guard<std::mutex> lock(mutex_);
    double sum = 0.0;
    for (auto& t : trades_) sum += t.pnl;
    return sum;
}

double ExchangeDB::winRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int total = 0;
    int wins = 0;
    for (auto& t : trades_) {
        if (t.pnl != 0.0) { ++total; if (t.pnl > 0.0) ++wins; }
    }
    return total > 0 ? static_cast<double>(wins) / total : 0.0;
}

} // namespace crypto
