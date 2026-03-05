#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <nlohmann/json.hpp>
#include "../data/AlertRepository.h"

namespace crypto {

struct Alert {
    std::string id;
    std::string symbol;
    std::string condition;  // "RSI_ABOVE" / "RSI_BELOW" / "PRICE_ABOVE" / "PRICE_BELOW" / "EMA_CROSS_UP" / "EMA_CROSS_DOWN"
    double      threshold{0.0};
    bool        enabled{true};
    bool        triggered{false};
    std::string notifyType; // "sound" / "telegram" / "both"
};

struct AlertTick {
    std::string symbol;
    double      close{0.0};
    double      rsi{0.0};
    double      emaFast{0.0};
    double      emaSlow{0.0};
};

class AlertManager {
public:
    // Telegram settings
    void setTelegramConfig(const std::string& token, const std::string& chatId);

    // Alert CRUD
    void addAlert(const Alert& alert);
    void removeAlert(const std::string& id);
    void updateAlert(const Alert& alert);
    void clearAll();

    std::vector<Alert> getAlerts() const;

    // Check alerts against current tick data
    void check(const AlertTick& tick);

    // Reset triggered state for an alert
    void resetAlert(const std::string& id);

    // Persistence
    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

    // Custom alert (from Pine Script)
    void fireCustomAlert(const std::string& message);

    // Set notification callback (for UI toast)
    using NotifyCallback = std::function<void(const std::string&)>;
    void setNotifyCallback(NotifyCallback cb) { onNotify_ = std::move(cb); }

    // Optional: set database repository for DB persistence
    void setRepository(AlertRepository* repo) { alertRepo_ = repo; }
    
    // Load alerts from database (if repo is set)
    void loadFromDB();

private:
    void notify(const Alert& alert, const AlertTick& tick);
    void sendTelegram(const std::string& msg);
    void playSound();

    mutable std::mutex mutex_;
    std::vector<Alert> alerts_;
    std::string telegramToken_;
    std::string telegramChatId_;
    NotifyCallback onNotify_;
    int nextId_{1};
    AlertRepository* alertRepo_{nullptr};
};

} // namespace crypto
