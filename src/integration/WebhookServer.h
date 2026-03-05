#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <chrono>
#include "../data/AuxRepository.h"

namespace crypto {

struct WebhookSignal {
    std::string action;   // "buy" / "sell"
    std::string symbol;
    double      qty{0.0};
    double      price{0.0};
};

struct WebhookResponse {
    int         status{200};
    std::string body;
};

class WebhookServer {
public:
    WebhookServer() = default;
    ~WebhookServer() = default;

    void setSecret(const std::string& secret);
    std::string getSecret() const;

    // Start/stop the HTTP listener (placeholder — real impl needs Boost.Beast)
    void start(uint16_t port = 8080);
    void stop();
    bool isRunning() const { return running_; }

    // Rate-limit check: max 10 requests per minute per IP
    bool checkRateLimit(const std::string& ip);

    // Simulate an incoming request (for testing without real HTTP)
    WebhookResponse simulateRequest(
        const std::string& secret,
        const std::string& body,
        const std::string& ip = "127.0.0.1");

    // Get last N received signals
    std::vector<WebhookSignal> getRecentSignals(int n = 20) const;

    void setPort(uint16_t p) { port_ = p; }
    uint16_t getPort() const { return port_; }

    void setAuxRepository(AuxRepository* repo) { auxRepo_ = repo; }

private:
    WebhookResponse handleRequest(
        const std::string& secret,
        const std::string& body,
        const std::string& ip);

    std::string     secretKey_;
    uint16_t        port_ = 8080;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;

    // Rate limiting: ip -> (count, windowStart)
    std::map<std::string, std::pair<int, long long>> rateLimit_;
    std::mutex rateMutex_;

    std::vector<WebhookSignal> recentSignals_;
    AuxRepository* auxRepo_{nullptr};
};

} // namespace crypto
