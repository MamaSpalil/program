#include "BybitExchange.h"
#include "../core/Logger.h"
#ifdef USE_CURL
#include <curl/curl.h>
#endif
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace crypto {

namespace {
#ifdef USE_CURL
size_t bybitWriteCb(char* ptr, size_t size, size_t nmemb, std::string* d) {
    d->append(ptr, size * nmemb);
    return size * nmemb;
}
#endif
std::string toHexB(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    return oss.str();
}
} // namespace

BybitExchange::BybitExchange(const std::string& apiKey,
                               const std::string& apiSecret,
                               const std::string& baseUrl,
                               const std::string& wsHost,
                               const std::string& wsPort)
    : apiKey_(apiKey), apiSecret_(apiSecret),
      baseUrl_(baseUrl), wsHost_(wsHost), wsPort_(wsPort) {}

BybitExchange::~BybitExchange() { disconnect(); }

std::string BybitExchange::sign(const std::string& payload) const {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    HMAC(EVP_sha256(),
         apiSecret_.c_str(), static_cast<int>(apiSecret_.size()),
         reinterpret_cast<const unsigned char*>(payload.c_str()),
         payload.size(), digest, &digestLen);
    return toHexB(digest, digestLen);
}

std::string BybitExchange::httpGet(const std::string& path) const {
#ifndef USE_CURL
    throw std::runtime_error("libcurl not available");
    (void)path;
#else
    std::string url = baseUrl_ + path;
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, bybitWriteCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
        throw std::runtime_error(std::string("curl: ") + curl_easy_strerror(res));
    if (code >= 400)
        throw std::runtime_error("HTTP " + std::to_string(code) + ": " + response);
    return response;
#endif
}

std::vector<Candle> BybitExchange::getKlines(const std::string& symbol,
                                               const std::string& interval,
                                               int limit) {
    Logger::get()->debug("[Bybit] getKlines symbol={} interval={} limit={}", symbol, interval, limit);
    std::string path = "/v5/market/kline?category=linear&symbol=" + symbol +
                       "&interval=" + interval + "&limit=" + std::to_string(limit);
    auto response = httpGet(path);
    Logger::get()->debug("[Bybit] getKlines response size={}", response.size());
    auto json = nlohmann::json::parse(response);
    std::vector<Candle> candles;
    if (json["retCode"].get<int>() != 0) {
        Logger::get()->warn("[Bybit] getKlines API error: retCode={}", json["retCode"].get<int>());
        return candles;
    }
    for (auto& k : json["result"]["list"]) {
        Candle c;
        c.openTime = std::stoll(k[0].get<std::string>());
        c.open     = std::stod(k[1].get<std::string>());
        c.high     = std::stod(k[2].get<std::string>());
        c.low      = std::stod(k[3].get<std::string>());
        c.close    = std::stod(k[4].get<std::string>());
        c.volume   = std::stod(k[5].get<std::string>());
        c.closed   = true;
        candles.push_back(c);
    }
    // Bybit v5 API returns candles newest-first; reverse to chronological order
    std::reverse(candles.begin(), candles.end());
    Logger::get()->debug("[Bybit] getKlines parsed {} candles", candles.size());
    return candles;
}

double BybitExchange::getPrice(const std::string& symbol) {
    Logger::get()->debug("[Bybit] getPrice symbol={}", symbol);
    auto json = nlohmann::json::parse(
        httpGet("/v5/market/tickers?category=linear&symbol=" + symbol));
    if (json["retCode"].get<int>() != 0) {
        Logger::get()->warn("[Bybit] getPrice API error: retCode={}", json["retCode"].get<int>());
        return 0.0;
    }
    double price = std::stod(json["result"]["list"][0]["lastPrice"].get<std::string>());
    Logger::get()->debug("[Bybit] getPrice result={}", price);
    return price;
}

OrderResponse BybitExchange::placeOrder(const OrderRequest& req) {
    // Simplified — real implementation would POST to /v5/order/create
    (void)req;
    return {};
}

AccountBalance BybitExchange::getBalance() {
    return {};
}

void BybitExchange::onWsMessage(const std::string& msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        if (!j.contains("data")) return;
        if (klineCb_) {
            auto& d = j["data"][0];
            Candle c;
            c.openTime = std::stoll(d["start"].get<std::string>());
            c.open     = std::stod(d["open"].get<std::string>());
            c.high     = std::stod(d["high"].get<std::string>());
            c.low      = std::stod(d["low"].get<std::string>());
            c.close    = std::stod(d["close"].get<std::string>());
            c.volume   = std::stod(d["volume"].get<std::string>());
            c.closed   = d["confirm"].get<bool>();
            klineCb_(c);
        }
    } catch (const std::exception& e) {
        Logger::get()->warn("Bybit WS parse: {}", e.what());
    }
}

void BybitExchange::subscribeKline(const std::string& symbol,
                                    const std::string& interval,
                                    std::function<void(const Candle&)> cb) {
    klineCb_ = std::move(cb);
    ws_ = std::make_unique<WebSocketClient>(wsHost_, wsPort_, "/v5/public/spot");
    ws_->setMessageCallback([this](const std::string& m){ onWsMessage(m); });
    (void)symbol; (void)interval;
}

void BybitExchange::connect() {
    Logger::get()->debug("[Bybit] Connecting WebSocket to {}:{}", wsHost_, wsPort_);
    if (ws_) ws_->connect();
}
void BybitExchange::disconnect() {
    Logger::get()->debug("[Bybit] Disconnecting");
    if (ws_) ws_->disconnect();
}

bool BybitExchange::testConnection(std::string& outError) {
    Logger::get()->debug("[Bybit] Testing connection to {}", baseUrl_);
    try {
        double price = getPrice("BTCUSDT");
        if (price > 0) {
            Logger::get()->info("[Bybit] Connection test OK, BTCUSDT={}", price);
            return true;
        }
        outError = "Received invalid price (0) from Bybit API";
        Logger::get()->warn("[Bybit] Connection test failed: {}", outError);
        return false;
    } catch (const std::exception& e) {
        outError = std::string("Bybit API error: ") + e.what();
        Logger::get()->warn("[Bybit] Connection test failed: {}", outError);
        return false;
    }
}

} // namespace crypto
