#include "AlertManager.h"
#include "../core/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef USE_CURL
#include <curl/curl.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

namespace crypto {

void AlertManager::setTelegramConfig(const std::string& token, const std::string& chatId) {
    std::lock_guard<std::mutex> lk(mutex_);
    telegramToken_ = token;
    telegramChatId_ = chatId;
}

void AlertManager::addAlert(const Alert& alert) {
    std::lock_guard<std::mutex> lk(mutex_);
    Alert a = alert;
    if (a.id.empty()) a.id = "alert_" + std::to_string(nextId_++);
    alerts_.push_back(a);
}

void AlertManager::removeAlert(const std::string& id) {
    std::lock_guard<std::mutex> lk(mutex_);
    alerts_.erase(std::remove_if(alerts_.begin(), alerts_.end(),
        [&](const Alert& a) { return a.id == id; }), alerts_.end());
}

void AlertManager::updateAlert(const Alert& alert) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& a : alerts_) {
        if (a.id == alert.id) {
            a = alert;
            return;
        }
    }
}

void AlertManager::clearAll() {
    std::lock_guard<std::mutex> lk(mutex_);
    alerts_.clear();
}

std::vector<Alert> AlertManager::getAlerts() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return alerts_;
}

void AlertManager::check(const AlertTick& tick) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& alert : alerts_) {
        if (!alert.enabled || alert.triggered) continue;
        if (!alert.symbol.empty() && alert.symbol != tick.symbol) continue;

        bool fired = false;
        if (alert.condition == "RSI_ABOVE")
            fired = tick.rsi > alert.threshold;
        else if (alert.condition == "RSI_BELOW")
            fired = tick.rsi < alert.threshold;
        else if (alert.condition == "PRICE_ABOVE")
            fired = tick.close > alert.threshold;
        else if (alert.condition == "PRICE_BELOW")
            fired = tick.close < alert.threshold;
        else if (alert.condition == "EMA_CROSS_UP")
            fired = tick.emaFast > tick.emaSlow;
        else if (alert.condition == "EMA_CROSS_DOWN")
            fired = tick.emaFast < tick.emaSlow;

        if (fired) {
            alert.triggered = true;
            notify(alert, tick);
        }
    }
}

void AlertManager::resetAlert(const std::string& id) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& a : alerts_) {
        if (a.id == id) { a.triggered = false; return; }
    }
}

void AlertManager::notify(const Alert& alert, const AlertTick& tick) {
    std::ostringstream msg;
    msg << "Alert: " << tick.symbol << " " << alert.condition
        << " " << alert.threshold
        << " | Price: $" << tick.close
        << " | RSI: " << tick.rsi;

    auto msgStr = msg.str();
    if (auto logger = Logger::get()) logger->info("Alert triggered: {}", msgStr);

    if (alert.notifyType == "telegram" || alert.notifyType == "both")
        sendTelegram(msgStr);
    if (alert.notifyType == "sound" || alert.notifyType == "both")
        playSound();
    if (onNotify_) onNotify_(msgStr);
}

void AlertManager::fireCustomAlert(const std::string& message) {
    if (auto logger = Logger::get()) logger->info("Custom alert: {}", message);
    if (onNotify_) onNotify_(message);
}

void AlertManager::sendTelegram(const std::string& msg) {
#ifdef USE_CURL
    if (telegramToken_.empty() || telegramChatId_.empty()) return;

    CURL* curl = curl_easy_init();
    if (!curl) return;

    // URL-encode the message
    char* encoded = curl_easy_escape(curl, msg.c_str(), static_cast<int>(msg.size()));
    std::string url = "https://api.telegram.org/bot" + telegramToken_
                    + "/sendMessage?chat_id=" + telegramChatId_
                    + "&text=" + std::string(encoded);
    curl_free(encoded);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    // Suppress response body
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char*, size_t size, size_t nmemb, void*) -> size_t { return size * nmemb; });

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        if (auto logger = Logger::get())
            logger->warn("Telegram send failed: {}", curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
#else
    (void)msg;
#endif
}

void AlertManager::playSound() {
#ifdef _WIN32
    MessageBeep(MB_ICONEXCLAMATION);
#endif
}

bool AlertManager::saveToFile(const std::string& path) const {
    std::lock_guard<std::mutex> lk(mutex_);
    try {
        nlohmann::json j;
        j["telegram_token"] = telegramToken_;
        j["telegram_chat_id"] = telegramChatId_;
        j["alerts"] = nlohmann::json::array();
        for (auto& a : alerts_) {
            j["alerts"].push_back({
                {"id", a.id}, {"symbol", a.symbol}, {"condition", a.condition},
                {"threshold", a.threshold}, {"enabled", a.enabled},
                {"triggered", a.triggered}, {"notifyType", a.notifyType}
            });
        }
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

bool AlertManager::loadFromFile(const std::string& path) {
    std::lock_guard<std::mutex> lk(mutex_);
    try {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        auto j = nlohmann::json::parse(f);
        telegramToken_ = j.value("telegram_token", "");
        telegramChatId_ = j.value("telegram_chat_id", "");
        alerts_.clear();
        if (j.contains("alerts")) {
            for (auto& ja : j["alerts"]) {
                Alert a;
                a.id = ja.value("id", "");
                a.symbol = ja.value("symbol", "");
                a.condition = ja.value("condition", "");
                a.threshold = ja.value("threshold", 0.0);
                a.enabled = ja.value("enabled", true);
                a.triggered = ja.value("triggered", false);
                a.notifyType = ja.value("notifyType", "sound");
                alerts_.push_back(a);
                // Track max ID
                constexpr size_t prefixLen = 6; // length of "alert_"
                if (a.id.size() > prefixLen) {
                    try {
                        int n = std::stoi(a.id.substr(prefixLen));
                        if (n >= nextId_) nextId_ = n + 1;
                    } catch (...) {}
                }
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace crypto
