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
                prof.testnet      = p.value("testnet", false);
                profiles_.push_back(std::move(prof));
            }
        }

        Logger::get()->info("[ExchangeDB] Loaded {} profile(s), active='{}'",
                            profiles_.size(), activeProfile_);
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
            pj["testnet"]       = p.testnet;
            arr.push_back(std::move(pj));
        }
        j["profiles"] = std::move(arr);

        std::ofstream f(dbPath_);
        if (!f.is_open()) {
            Logger::get()->warn("[ExchangeDB] Cannot write {}", dbPath_);
            return false;
        }
        f << j.dump(2);
        Logger::get()->debug("[ExchangeDB] Saved {} profile(s) to {}", profiles_.size(), dbPath_);
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

} // namespace crypto
