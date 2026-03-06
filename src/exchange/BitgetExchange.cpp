#include "BitgetExchange.h"
#include "../core/Logger.h"
#ifdef USE_CURL
#include <curl/curl.h>
#endif
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <algorithm>
#include <stdexcept>

namespace crypto {

namespace {
inline double safeStod(const std::string& s, double defaultVal = 0.0) {
    if (s.empty()) return defaultVal;
    try { return std::stod(s); } catch (...) { return defaultVal; }
}
} // namespace

#ifdef USE_CURL
namespace {
size_t bitgetWriteCb(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}
} // namespace
#endif

namespace {
std::string base64Encode(const unsigned char* data, size_t len) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data, static_cast<int>(len));
    BIO_flush(b64);
    BUF_MEM* bptr = nullptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
}

} // namespace

BitgetExchange::BitgetExchange(const std::string& apiKey,
                                const std::string& apiSecret,
                                const std::string& passphrase,
                                const std::string& baseUrl,
                                const std::string& wsHost,
                                const std::string& wsPort)
    : apiKey_(apiKey), apiSecret_(apiSecret), passphrase_(passphrase),
      baseUrl_(baseUrl), wsHost_(wsHost), wsPort_(wsPort) {}

BitgetExchange::~BitgetExchange() { disconnect(); }

std::string BitgetExchange::sign(const std::string& timestamp,
                                  const std::string& method,
                                  const std::string& path) const {
    std::string prehash = timestamp + method + path;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    HMAC(EVP_sha256(),
         apiSecret_.c_str(), static_cast<int>(apiSecret_.size()),
         reinterpret_cast<const unsigned char*>(prehash.c_str()),
         prehash.size(), digest, &digestLen);
    return base64Encode(digest, digestLen);
}

std::string BitgetExchange::sign(const std::string& timestamp,
                                  const std::string& method,
                                  const std::string& path,
                                  const std::string& body) const {
    std::string prehash = timestamp + method + path + body;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    HMAC(EVP_sha256(),
         apiSecret_.c_str(), static_cast<int>(apiSecret_.size()),
         reinterpret_cast<const unsigned char*>(prehash.c_str()),
         prehash.size(), digest, &digestLen);
    return base64Encode(digest, digestLen);
}

void BitgetExchange::rateLimit() const {
    std::lock_guard<std::mutex> lock(rateMutex_);
    auto now = std::chrono::steady_clock::now();
    auto windowStart = now - std::chrono::seconds(1);

    while (!requestTimes_.empty() && requestTimes_.front() < windowStart) {
        requestTimes_.pop();
    }
    if (static_cast<int>(requestTimes_.size()) >= MAX_REQUESTS_PER_SECOND) {
        auto sleepUntil = requestTimes_.front() + std::chrono::seconds(1);
        std::this_thread::sleep_until(sleepUntil);
    }
    requestTimes_.push(now);
}

std::string BitgetExchange::httpGet(const std::string& path, bool signed_) const {
#ifndef USE_CURL
    throw std::runtime_error("libcurl not available");
    (void)path; (void)signed_;
#else
    rateLimit();
    std::string url = baseUrl_ + path;

    int retries = 3;
    while (retries-- > 0) {
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl_easy_init failed");

        std::string response;
        struct curl_slist* headers = nullptr;

        if (signed_) {
            auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            std::string timestamp = std::to_string(ts);

            std::string sig = sign(timestamp, "GET", path);
            headers = curl_slist_append(headers, ("ACCESS-KEY: " + apiKey_).c_str());
            headers = curl_slist_append(headers, ("ACCESS-SIGN: " + sig).c_str());
            headers = curl_slist_append(headers, ("ACCESS-TIMESTAMP: " + timestamp).c_str());
            headers = curl_slist_append(headers, ("ACCESS-PASSPHRASE: " + passphrase_).c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (headers)
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, bitgetWriteCb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK && httpCode < 400) return response;

        if (httpCode == 429) {
            std::this_thread::sleep_for(std::chrono::seconds(5 * (3 - retries)));
        } else if (res != CURLE_OK) {
            Logger::get()->warn("HTTP GET error: {}", curl_easy_strerror(res));
        } else {
            throw std::runtime_error("HTTP " + std::to_string(httpCode) + ": " + response);
        }
    }
    throw std::runtime_error("Max retries exceeded");
#endif
}

std::vector<Candle> BitgetExchange::getKlines(const std::string& symbol,
                                               const std::string& interval,
                                               int limit) {
    // Bitget API max limit is 1000
    if (limit > 1000) {
        Logger::get()->debug("[Bitget] getKlines limit {} clamped to 1000", limit);
        limit = 1000;
    }
    Logger::get()->debug("[Bitget] getKlines symbol={} interval={} limit={}", symbol, interval, limit);
    std::string path = "/api/v2/mix/market/candles?symbol=" + symbol +
                       "&productType=USDT-FUTURES" +
                       "&granularity=" + interval +
                       "&limit=" + std::to_string(limit);
    auto response = httpGet(path);
    Logger::get()->debug("[Bitget] getKlines response size={}", response.size());
    auto json = nlohmann::json::parse(response);
    std::vector<Candle> candles;
    if (json["code"].get<std::string>() != "00000") {
        Logger::get()->warn("[Bitget] getKlines API error: code={} msg={}",
            json["code"].get<std::string>(), json.value("msg", ""));
        return candles;
    }
    for (auto& k : json["data"]) {
        Candle c;
        c.openTime = std::stoll(k[0].get<std::string>());
        c.open     = safeStod(k[1].get<std::string>());
        c.high     = safeStod(k[2].get<std::string>());
        c.low      = safeStod(k[3].get<std::string>());
        c.close    = safeStod(k[4].get<std::string>());
        c.volume   = safeStod(k[5].get<std::string>());
        c.closed   = true;
        candles.push_back(c);
    }
    Logger::get()->debug("[Bitget] getKlines parsed {} candles", candles.size());
    return candles;
}

double BitgetExchange::getPrice(const std::string& symbol) {
    Logger::get()->debug("[Bitget] getPrice symbol={}", symbol);
    auto json = nlohmann::json::parse(
        httpGet("/api/v2/mix/market/ticker?symbol=" + symbol + "&productType=USDT-FUTURES"));
    if (json["code"].get<std::string>() != "00000") {
        Logger::get()->warn("[Bitget] getPrice API error: code={}", json["code"].get<std::string>());
        return 0.0;
    }
    double price = safeStod(json["data"][0]["lastPr"].get<std::string>());
    Logger::get()->debug("[Bitget] getPrice result={}", price);
    return price;
}

OrderResponse BitgetExchange::placeOrder(const OrderRequest& req) {
#ifndef USE_CURL
    (void)req;
    return {};
#else
    try {
        std::string side = (req.side == OrderRequest::Side::BUY) ? "buy" : "sell";
        std::string orderType;
        switch (req.type) {
            case OrderRequest::Type::MARKET:    orderType = "market"; break;
            case OrderRequest::Type::LIMIT:     orderType = "limit"; break;
            case OrderRequest::Type::STOP_LOSS: orderType = "market"; break;
        }

        nlohmann::json body;
        body["symbol"] = req.symbol;
        body["side"] = side;
        body["orderType"] = orderType;
        body["size"] = std::to_string(req.qty);
        if (req.type == OrderRequest::Type::LIMIT && req.price > 0)
            body["price"] = std::to_string(req.price);

        auto resp = httpPost("/api/v2/spot/trade/place-order", body.dump());
        auto j = nlohmann::json::parse(resp);

        OrderResponse result;
        if (j.value("code", "") != "00000") {
            Logger::get()->warn("[Bitget] placeOrder error: code={} msg={}",
                                j.value("code", ""), j.value("msg", ""));
            result.status = "ERROR";
            return result;
        }
        if (j.contains("data")) {
            result.orderId = j["data"].value("orderId", "");
        }
        result.status = "NEW";
        result.executedQty = req.qty;
        result.executedPrice = req.price;
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bitget] placeOrder failed: {}", e.what());
        OrderResponse result;
        result.status = "ERROR";
        return result;
    }
#endif
}

std::string BitgetExchange::httpPost(const std::string& path,
                                      const std::string& body) const {
#ifndef USE_CURL
    throw std::runtime_error("libcurl not available");
    (void)path; (void)body;
#else
    rateLimit();
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string timestamp = std::to_string(ts);

    std::string sig = sign(timestamp, "POST", path, body);

    std::string url = baseUrl_ + path;
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("ACCESS-KEY: " + apiKey_).c_str());
    headers = curl_slist_append(headers, ("ACCESS-SIGN: " + sig).c_str());
    headers = curl_slist_append(headers, ("ACCESS-TIMESTAMP: " + timestamp).c_str());
    headers = curl_slist_append(headers, ("ACCESS-PASSPHRASE: " + passphrase_).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, bitgetWriteCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error(std::string("curl: ") + curl_easy_strerror(res));
    if (httpCode >= 400)
        throw std::runtime_error("HTTP POST " + std::to_string(httpCode) + ": " + response);
    return response;
#endif
}

AccountBalance BitgetExchange::getBalance() {
#ifndef USE_CURL
    return {};
#else
    try {
        auto resp = httpGet("/api/v2/spot/account/assets", true);
        auto j = nlohmann::json::parse(resp);
        AccountBalance bal;
        if (j.contains("data") && j["data"].is_array()) {
            for (auto& asset : j["data"]) {
                std::string coin = asset.value("coin", "");
                if (coin == "USDT") {
                    bal.totalUSDT     = safeStod(asset.value("available", "0"))
                                      + safeStod(asset.value("frozen", "0"));
                    bal.availableUSDT = safeStod(asset.value("available", "0"));
                }
                if (coin == "BTC") {
                    bal.btcBalance = safeStod(asset.value("available", "0"));
                }
            }
        }
        return bal;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bitget] getBalance failed: {}", e.what());
        return {};
    }
#endif
}

void BitgetExchange::onWsMessage(const std::string& msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        if (!j.contains("data")) return;
        if (klineCb_) {
            for (auto& k : j["data"]) {
                Candle c;
                c.openTime = std::stoll(k[0].get<std::string>());
                c.open     = safeStod(k[1].get<std::string>());
                c.high     = safeStod(k[2].get<std::string>());
                c.low      = safeStod(k[3].get<std::string>());
                c.close    = safeStod(k[4].get<std::string>());
                c.volume   = safeStod(k[5].get<std::string>());
                c.closed   = true;
                klineCb_(c);
            }
        }
    } catch (const std::exception& e) {
        Logger::get()->warn("Bitget WS parse: {}", e.what());
    }
}

void BitgetExchange::subscribeKline(const std::string& symbol,
                                     const std::string& interval,
                                     std::function<void(const Candle&)> cb) {
    klineCb_ = std::move(cb);
    ws_ = std::make_unique<WebSocketClient>(wsHost_, wsPort_, "/v2/ws/public");
    ws_->setMessageCallback([this](const std::string& m){ onWsMessage(m); });
    ws_->connect();
    // Send subscription message
    nlohmann::json sub;
    sub["op"] = "subscribe";
    sub["args"] = nlohmann::json::array();
    nlohmann::json arg;
    arg["instType"] = "SPOT";
    arg["channel"] = "candle" + interval;
    arg["instId"] = symbol;
    sub["args"].push_back(arg);
    ws_->send(sub.dump());
}

void BitgetExchange::connect() {
    Logger::get()->debug("[Bitget] Connecting WebSocket to {}:{}", wsHost_, wsPort_);
    if (ws_) ws_->connect();
}
void BitgetExchange::disconnect() {
    Logger::get()->debug("[Bitget] Disconnecting");
    if (ws_) ws_->disconnect();
}

bool BitgetExchange::testConnection(std::string& outError) {
    Logger::get()->debug("[Bitget] Testing connection to {}", baseUrl_);
    try {
        double price = getPrice("BTCUSDT");
        if (price > 0) {
            Logger::get()->info("[Bitget] Connection test OK, BTCUSDT={}", price);
            return true;
        }
        outError = "Received invalid price (0) from Bitget API";
        Logger::get()->warn("[Bitget] Connection test failed: {}", outError);
        return false;
    } catch (const std::exception& e) {
        outError = std::string("Bitget API error: ") + e.what();
        Logger::get()->warn("[Bitget] Connection test failed: {}", outError);
        return false;
    }
}

std::vector<SymbolInfo> BitgetExchange::getSymbols(const std::string& marketType) {
#ifndef USE_CURL
    (void)marketType;
    return {};
#else
    std::vector<SymbolInfo> result;
    try {
        // Spot symbols
        if (marketType.empty() || marketType == "spot") {
            auto resp = httpGet("/api/v2/spot/public/symbols");
            auto j = nlohmann::json::parse(resp);
            if (j.contains("data") && j["data"].is_array()) {
                for (auto& s : j["data"]) {
                    SymbolInfo si;
                    si.symbol     = s.value("symbol", "");
                    si.baseAsset  = s.value("baseCoin", "");
                    si.quoteAsset = s.value("quoteCoin", "");
                    si.marketType = "spot";
                    si.displayName = si.baseAsset + "/" + si.quoteAsset;
                    result.push_back(std::move(si));
                }
            }
        }
        // Futures (USDT-FUTURES) symbols
        if (marketType.empty() || marketType == "futures") {
            auto resp = httpGet("/api/v2/mix/market/contracts?productType=USDT-FUTURES");
            auto j = nlohmann::json::parse(resp);
            if (j.contains("data") && j["data"].is_array()) {
                for (auto& s : j["data"]) {
                    SymbolInfo si;
                    si.symbol     = s.value("symbol", "");
                    si.baseAsset  = s.value("baseCoin", "");
                    si.quoteAsset = s.value("quoteCoin", "");
                    si.marketType = "futures";
                    si.displayName = si.baseAsset + "/" + si.quoteAsset + " Swap";
                    result.push_back(std::move(si));
                }
            }
        }
        Logger::get()->info("[Bitget] Fetched {} symbols", result.size());
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bitget] getSymbols failed: {}", e.what());
    }
    return result;
#endif
}

OrderBook BitgetExchange::getOrderBook(const std::string& symbol, int depth) {
    OrderBook ob;
#ifdef USE_CURL
    try {
        Logger::get()->debug("[Bitget] getOrderBook symbol={} depth={}", symbol, depth);
        std::string path = "/api/v2/mix/market/merge-depth?symbol=" + symbol +
                           "&productType=USDT-FUTURES" +
                           "&limit=" + std::to_string(depth);
        auto response = httpGet(path);
        auto json = nlohmann::json::parse(response);
        if (json["code"].get<std::string>() != "00000") {
            Logger::get()->warn("[Bitget] getOrderBook API error: code={}", json["code"].get<std::string>());
            return ob;
        }
        auto& data = json["data"];
        for (auto& b : data["bids"]) {
            OrderBook::Level lvl;
            lvl.price = safeStod(b[0].get<std::string>());
            lvl.qty   = safeStod(b[1].get<std::string>());
            ob.bids.push_back(lvl);
        }
        for (auto& a : data["asks"]) {
            OrderBook::Level lvl;
            lvl.price = safeStod(a[0].get<std::string>());
            lvl.qty   = safeStod(a[1].get<std::string>());
            ob.asks.push_back(lvl);
        }
        Logger::get()->debug("[Bitget] getOrderBook bids={} asks={}", ob.bids.size(), ob.asks.size());
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bitget] getOrderBook failed: {}", e.what());
    }
#else
    (void)symbol; (void)depth;
#endif
    return ob;
}

} // namespace crypto
