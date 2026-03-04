#include "TelegramBot.h"
#include "../core/Logger.h"
#include <chrono>

namespace crypto {

TelegramBot::~TelegramBot() {
    stopPolling();
}

void TelegramBot::startPolling() {
    if (polling_) return;
    polling_ = true;
    pollThread_ = std::thread(&TelegramBot::pollLoop, this);
}

void TelegramBot::stopPolling() {
    polling_ = false;
    if (pollThread_.joinable()) pollThread_.join();
}

std::string TelegramBot::processCommand(const std::string& text,
                                         const std::string& chatId) {
    // Security: only authorized chat
    if (chatId != authorizedChatId_) {
        return "Unauthorized";
    }

    if (text == "/help") {
        return "/balance - account balance\n"
               "/status  - engine status\n"
               "/positions - open positions\n"
               "/pnl - daily P&L\n"
               "/buy SYMBOL QTY\n"
               "/sell SYMBOL QTY\n"
               "/stop - stop engine\n"
               "/start - start engine\n"
               "/help - this help";
    }
    if (text == "/balance") {
        return "Balance: (requires exchange connection)";
    }
    if (text == "/status") {
        return "Status: idle";
    }
    if (text == "/positions") {
        return "No open positions";
    }
    if (text == "/pnl") {
        return "Daily P&L: $0.00";
    }
    if (text == "/stop") {
        return "Engine stopped";
    }
    if (text == "/start") {
        return "Engine started";
    }
    if (text.rfind("/buy ", 0) == 0 || text.rfind("/sell ", 0) == 0) {
        return "Order command received (requires risk check)";
    }

    return "Unknown command. Use /help";
}

bool TelegramBot::sendMessage(const std::string& chatId, const std::string& message) {
    // Placeholder: POST to https://api.telegram.org/bot{token}/sendMessage
    Logger::get()->info("[TelegramBot] -> {} : {}", chatId, message);
    (void)chatId;
    (void)message;
    return true;
}

std::vector<std::string> TelegramBot::availableCommands() {
    return {"/help", "/balance", "/status", "/positions", "/pnl",
            "/buy", "/sell", "/stop", "/start"};
}

std::vector<TelegramUpdate> TelegramBot::getUpdates(long long offset) {
    // Placeholder: GET https://api.telegram.org/bot{token}/getUpdates?offset={offset}&timeout=30
    (void)offset;
    return {};
}

void TelegramBot::pollLoop() {
    while (polling_) {
        auto updates = getUpdates(lastUpdateId_ + 1);
        for (auto& u : updates) {
            lastUpdateId_ = u.updateId;
            auto reply = processCommand(u.text, u.chatId);
            sendMessage(u.chatId, reply);
        }
        // Sleep 5 seconds between polls
        for (int i = 0; i < 5 && polling_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

} // namespace crypto
