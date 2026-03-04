#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

namespace crypto {

struct TelegramUpdate {
    long long   updateId{0};
    std::string text;
    std::string chatId;
};

class TelegramBot {
public:
    TelegramBot() = default;
    ~TelegramBot();

    void setToken(const std::string& token) { token_ = token; }
    void setAuthorizedChat(const std::string& chatId) { authorizedChatId_ = chatId; }

    void startPolling();
    void stopPolling();
    bool isPolling() const { return polling_; }

    // Process a command — public for testing
    std::string processCommand(const std::string& text, const std::string& chatId);

    // Send a message to a chat
    bool sendMessage(const std::string& chatId, const std::string& message);

    // Get available commands
    static std::vector<std::string> availableCommands();

private:
    void pollLoop();
    std::vector<TelegramUpdate> getUpdates(long long offset);

    std::string     token_;
    std::string     authorizedChatId_;
    long long       lastUpdateId_{0};
    std::thread     pollThread_;
    std::atomic<bool> polling_{false};
    mutable std::mutex mutex_;
};

} // namespace crypto
