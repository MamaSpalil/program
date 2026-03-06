#include "TelegramBot.h"
#include "../core/Logger.h"
#include <chrono>
#ifdef USE_CURL
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#endif

namespace crypto {

#ifdef USE_CURL
namespace {
size_t telegramWriteCallback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}
} // namespace
#endif

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
    if (token_.empty()) {
        Logger::get()->info("[TelegramBot] -> {} : {} (no token configured)", chatId, message);
        return true;
    }

#ifdef USE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger::get()->warn("[TelegramBot] curl_easy_init failed");
        return false;
    }

    std::string url = "https://api.telegram.org/bot" + token_ + "/sendMessage";

    nlohmann::json body;
    body["chat_id"] = chatId;
    body["text"] = message;
    body["parse_mode"] = "HTML";
    std::string bodyStr = body.dump();

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, telegramWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        Logger::get()->warn("[TelegramBot] curl error: {}", curl_easy_strerror(res));
        return false;
    }

    if (httpCode == 200) {
        Logger::get()->debug("[TelegramBot] Message sent to {}", chatId);
        return true;
    } else {
        Logger::get()->warn("[TelegramBot] HTTP {} — response: {}", httpCode, response);
        return false;
    }
#else
    Logger::get()->info("[TelegramBot] -> {} : {} (libcurl not available)", chatId, message);
    return true;
#endif
}

std::vector<std::string> TelegramBot::availableCommands() {
    return {"/help", "/balance", "/status", "/positions", "/pnl",
            "/buy", "/sell", "/stop", "/start"};
}

std::vector<TelegramUpdate> TelegramBot::getUpdates(long long offset) {
    if (token_.empty()) return {};

#ifdef USE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) return {};

    std::string url = "https://api.telegram.org/bot" + token_
                    + "/getUpdates?offset=" + std::to_string(offset) + "&timeout=30";
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, telegramWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 35L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpCode != 200) return {};

    std::vector<TelegramUpdate> updates;
    try {
        auto json = nlohmann::json::parse(response);
        if (!json.value("ok", false)) return {};
        for (const auto& r : json.value("result", nlohmann::json::array())) {
            TelegramUpdate u;
            u.updateId = r.value("update_id", 0LL);
            if (r.contains("message")) {
                const auto& msg = r["message"];
                u.text   = msg.value("text", "");
                u.chatId = std::to_string(msg.value("chat", nlohmann::json{}).value("id", 0LL));
            }
            updates.push_back(std::move(u));
        }
    } catch (const std::exception& e) {
        Logger::get()->warn("[TelegramBot] getUpdates parse error: {}", e.what());
    }
    return updates;
#else
    (void)offset;
    return {};
#endif
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
