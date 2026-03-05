#include "WebhookServer.h"
#include "../core/Logger.h"
#include <nlohmann/json.hpp>
#include <chrono>

namespace crypto {

void WebhookServer::setSecret(const std::string& secret) {
    std::lock_guard<std::mutex> lk(mutex_);
    secretKey_ = secret;
}

std::string WebhookServer::getSecret() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return secretKey_;
}

void WebhookServer::start(uint16_t port) {
    port_ = port;
    running_ = true;
    Logger::get()->info("[Webhook] Server started on port {}", port);
}

void WebhookServer::stop() {
    running_ = false;
    Logger::get()->info("[Webhook] Server stopped");
}

bool WebhookServer::checkRateLimit(const std::string& ip) {
    std::lock_guard<std::mutex> lk(rateMutex_);
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    auto& [count, windowStart] = rateLimit_[ip];
    if (now - windowStart > 60) {
        count = 0;
        windowStart = now;
    }
    return ++count <= 10;
}

WebhookResponse WebhookServer::handleRequest(
    const std::string& secret,
    const std::string& body,
    const std::string& ip)
{
    WebhookResponse res;

    // Rate limit check
    if (!checkRateLimit(ip)) {
        res.status = 429;
        res.body = "{\"error\":\"rate limited\"}";
        Logger::get()->warn("[Webhook] Rate limit exceeded for {}", ip);
        return res;
    }

    // Secret validation
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (secret != secretKey_) {
            res.status = 401;
            res.body = "{\"error\":\"unauthorized\"}";
            Logger::get()->warn("[Webhook] Bad secret from {}", ip);
            return res;
        }
    }

    // Parse body
    try {
        auto j = nlohmann::json::parse(body);
        WebhookSignal sig;
        sig.action = j.value("action", "");
        sig.symbol = j.value("symbol", "");
        sig.qty    = j.value("qty", 0.0);
        sig.price  = j.value("price", 0.0);

        {
            std::lock_guard<std::mutex> lk(mutex_);
            recentSignals_.insert(recentSignals_.begin(), sig);
            if (recentSignals_.size() > 100)
                recentSignals_.resize(100);
        }

        // Log to database
        if (auxRepo_) {
            WebhookLogEntry logEntry;
            logEntry.receivedAt = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            logEntry.action = sig.action;
            logEntry.symbol = sig.symbol;
            logEntry.qty = sig.qty;
            logEntry.price = sig.price;
            logEntry.status = "ok";
            logEntry.clientIp = ip;
            auxRepo_->logWebhook(logEntry);
        }

        res.status = 200;
        res.body = "{\"status\":\"received\"}";
        Logger::get()->info("[Webhook] Signal: {} {} qty={}",
                            sig.action, sig.symbol, sig.qty);
    } catch (const std::exception& e) {
        res.status = 400;
        res.body = "{\"error\":\"bad request\"}";
        Logger::get()->warn("[Webhook] Parse error: {}", e.what());
    }

    return res;
}

WebhookResponse WebhookServer::simulateRequest(
    const std::string& secret,
    const std::string& body,
    const std::string& ip)
{
    return handleRequest(secret, body, ip);
}

std::vector<WebhookSignal> WebhookServer::getRecentSignals(int n) const {
    std::lock_guard<std::mutex> lk(mutex_);
    int count = std::min(n, static_cast<int>(recentSignals_.size()));
    return std::vector<WebhookSignal>(
        recentSignals_.begin(),
        recentSignals_.begin() + count);
}

} // namespace crypto
