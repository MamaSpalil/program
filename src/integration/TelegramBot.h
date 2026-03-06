#pragma once
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

namespace crypto {

struct TelegramUpdate {
    long long   updateId{0};
    std::string text;
    std::string chatId;
};

/// Callback type for command data providers.
/// Returns the response string for a given command.
using TelegramCommandCallback = std::function<std::string()>;

class TelegramBot {
public:
    TelegramBot() = default;
    ~TelegramBot();

    void setToken(const std::string& token) { token_ = token; }
    void setAuthorizedChat(const std::string& chatId) { authorizedChatId_ = chatId; }

    void startPolling();
    void stopPolling();
    bool isPolling() const { return polling_; }

    /// Register a callback that provides data for a command.
    /// Example: setCommandCallback("/balance", [&]{ return "USDT: 1000.0"; });
    void setCommandCallback(const std::string& command, TelegramCommandCallback cb);

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

    // Command callbacks for dynamic data
    std::mutex callbackMutex_;
    std::map<std::string, TelegramCommandCallback> commandCallbacks_;
};

} // namespace crypto
