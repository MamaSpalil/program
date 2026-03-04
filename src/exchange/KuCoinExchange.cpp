#include "KuCoinExchange.h"
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
size_t kucoinWriteCb(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}
} // namespace
#endif

namespace {
std::string base64EncodeKc(const unsigned char* data, size_t len) {
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

KuCoinExchange::KuCoinExchange(const std::string& apiKey,
                                 const std::string& apiSecret,
                                 const std::string& passphrase,
                                 const std::string& baseUrl,
                                 const std::string& wsHost,
                                 const std::string& wsPort)
    : apiKey_(apiKey), apiSecret_(apiSecret), passphrase_(passphrase),
      baseUrl_(baseUrl), wsHost_(wsHost), wsPort_(wsPort) {}

KuCoinExchange::~KuCoinExchange() { disconnect(); }

std::string KuCoinExchange::sign(const std::string& timestamp,
                                   const std::string& method,
                                   const std::string& path) const {
    std::string prehash = timestamp + method + path;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    HMAC(EVP_sha256(),
         apiSecret_.c_str(), static_cast<int>(apiSecret_.size()),
         reinterpret_cast<const unsigned char*>(prehash.c_str()),
         prehash.size(), digest, &digestLen);
    return base64EncodeKc(digest, digestLen);
}

void KuCoinExchange::rateLimit() const {
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

std::string KuCoinExchange::httpGet(const std::string& path, bool signed_) const {
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

            // Sign the passphrase with HMAC-SHA256 as required by KuCoin
            unsigned char ppDigest[EVP_MAX_MD_SIZE];
            unsigned int ppDigestLen = 0;
            HMAC(EVP_sha256(),
                 apiSecret_.c_str(), static_cast<int>(apiSecret_.size()),
                 reinterpret_cast<const unsigned char*>(passphrase_.c_str()),
                 passphrase_.size(), ppDigest, &ppDigestLen);
            std::string signedPassphrase = base64EncodeKc(ppDigest, ppDigestLen);

            headers = curl_slist_append(headers, ("KC-API-KEY: " + apiKey_).c_str());
            headers = curl_slist_append(headers, ("KC-API-SIGN: " + sig).c_str());
            headers = curl_slist_append(headers, ("KC-API-TIMESTAMP: " + timestamp).c_str());
            headers = curl_slist_append(headers, ("KC-API-PASSPHRASE: " + signedPassphrase).c_str());
            headers = curl_slist_append(headers, "KC-API-KEY-VERSION: 2");
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (headers)
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, kucoinWriteCb);
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

std::vector<Candle> KuCoinExchange::getKlines(const std::string& symbol,
                                                const std::string& interval,
                                                int limit) {
    Logger::get()->debug("[KuCoin] getKlines symbol={} interval={} limit={}", symbol, interval, limit);
    (void)limit;
    std::string path = "/api/v1/market/candles?type=" + interval +
                       "&symbol=" + symbol;
    auto response = httpGet(path);
    Logger::get()->debug("[KuCoin] getKlines response size={}", response.size());
    auto json = nlohmann::json::parse(response);
    std::vector<Candle> candles;
    if (json["code"].get<std::string>() != "200000") {
        Logger::get()->warn("[KuCoin] getKlines API error: code={} msg={}",
            json["code"].get<std::string>(), json.value("msg", ""));
        return candles;
    }
    for (auto& k : json["data"]) {
        Candle c;
        c.openTime = std::stoll(k[0].get<std::string>());
        c.open     = safeStod(k[1].get<std::string>());
        c.close    = safeStod(k[2].get<std::string>());
        c.high     = safeStod(k[3].get<std::string>());
        c.low      = safeStod(k[4].get<std::string>());
        c.volume   = safeStod(k[5].get<std::string>());
        c.closed   = true;
        candles.push_back(c);
    }
    Logger::get()->debug("[KuCoin] getKlines parsed {} candles", candles.size());
    return candles;
}

double KuCoinExchange::getPrice(const std::string& symbol) {
    Logger::get()->debug("[KuCoin] getPrice symbol={}", symbol);
    auto json = nlohmann::json::parse(
        httpGet("/api/v1/market/orderbook/level1?symbol=" + symbol));
    if (json["code"].get<std::string>() != "200000") {
        Logger::get()->warn("[KuCoin] getPrice API error: code={}", json["code"].get<std::string>());
        return 0.0;
    }
    double price = safeStod(json["data"]["price"].get<std::string>());
    Logger::get()->debug("[KuCoin] getPrice result={}", price);
    return price;
}

OrderResponse KuCoinExchange::placeOrder(const OrderRequest& req) {
    // Simplified — real implementation would POST to /api/v1/orders
    (void)req;
    return {};
}

AccountBalance KuCoinExchange::getBalance() {
    return {};
}

void KuCoinExchange::onWsMessage(const std::string& msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        if (!j.contains("data")) return;
        if (klineCb_) {
            auto& d = j["data"]["candles"];
            Candle c;
            c.openTime = std::stoll(d[0].get<std::string>());
            c.open     = safeStod(d[1].get<std::string>());
            c.close    = safeStod(d[2].get<std::string>());
            c.high     = safeStod(d[3].get<std::string>());
            c.low      = safeStod(d[4].get<std::string>());
            c.volume   = safeStod(d[5].get<std::string>());
            c.closed   = false;
            klineCb_(c);
        }
    } catch (const std::exception& e) {
        Logger::get()->warn("KuCoin WS parse: {}", e.what());
    }
}

void KuCoinExchange::subscribeKline(const std::string& symbol,
                                      const std::string& interval,
                                      std::function<void(const Candle&)> cb) {
    klineCb_ = std::move(cb);
    ws_ = std::make_unique<WebSocketClient>(wsHost_, wsPort_, "/endpoint");
    ws_->setMessageCallback([this](const std::string& m){ onWsMessage(m); });
    (void)symbol; (void)interval;
}

void KuCoinExchange::connect() {
    Logger::get()->debug("[KuCoin] Connecting WebSocket to {}:{}", wsHost_, wsPort_);
    if (ws_) ws_->connect();
}
void KuCoinExchange::disconnect() {
    Logger::get()->debug("[KuCoin] Disconnecting");
    if (ws_) ws_->disconnect();
}

bool KuCoinExchange::testConnection(std::string& outError) {
    Logger::get()->debug("[KuCoin] Testing connection to {}", baseUrl_);
    try {
        double price = getPrice("BTC-USDT");
        if (price > 0) {
            Logger::get()->info("[KuCoin] Connection test OK, BTC-USDT={}", price);
            return true;
        }
        outError = "Received invalid price (0) from KuCoin API";
        Logger::get()->warn("[KuCoin] Connection test failed: {}", outError);
        return false;
    } catch (const std::exception& e) {
        outError = std::string("KuCoin API error: ") + e.what();
        Logger::get()->warn("[KuCoin] Connection test failed: {}", outError);
        return false;
    }
}

std::vector<SymbolInfo> KuCoinExchange::getSymbols(const std::string& marketType) {
#ifndef USE_CURL
    (void)marketType;
    return {};
#else
    std::vector<SymbolInfo> result;
    try {
        // Spot symbols
        if (marketType.empty() || marketType == "spot") {
            auto resp = httpGet("/api/v2/symbols");
            auto j = nlohmann::json::parse(resp);
            if (j.contains("data") && j["data"].is_array()) {
                for (auto& s : j["data"]) {
                    if (!s.value("enableTrading", true)) continue;
                    SymbolInfo si;
                    si.symbol     = s.value("symbol", "");
                    si.baseAsset  = s.value("baseCurrency", "");
                    si.quoteAsset = s.value("quoteCurrency", "");
                    si.marketType = "spot";
                    si.displayName = si.baseAsset + "/" + si.quoteAsset;
                    result.push_back(std::move(si));
                }
            }
        }
        // Futures symbols
        if (marketType.empty() || marketType == "futures") {
            auto resp = httpGet("/api/v1/contracts/active");
            auto j = nlohmann::json::parse(resp);
            if (j.contains("data") && j["data"].is_array()) {
                for (auto& s : j["data"]) {
                    SymbolInfo si;
                    si.symbol     = s.value("symbol", "");
                    si.baseAsset  = s.value("baseCurrency", "");
                    si.quoteAsset = s.value("quoteCurrency", "");
                    si.marketType = "futures";
                    si.displayName = si.baseAsset + "/" + si.quoteAsset + " Swap";
                    result.push_back(std::move(si));
                }
            }
        }
        Logger::get()->info("[KuCoin] Fetched {} symbols", result.size());
    } catch (const std::exception& e) {
        Logger::get()->warn("[KuCoin] getSymbols failed: {}", e.what());
    }
    return result;
#endif
}

OrderBook KuCoinExchange::getOrderBook(const std::string& symbol, int depth) {
    OrderBook ob;
#ifdef USE_CURL
    try {
        Logger::get()->debug("[KuCoin] getOrderBook symbol={} depth={}", symbol, depth);
        std::string path = "/api/v1/market/orderbook/level2_" +
                           std::to_string(depth) + "?symbol=" + symbol;
        auto response = httpGet(path);
        auto json = nlohmann::json::parse(response);
        if (json["code"].get<std::string>() != "200000") {
            Logger::get()->warn("[KuCoin] getOrderBook API error: code={}", json["code"].get<std::string>());
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
        Logger::get()->debug("[KuCoin] getOrderBook bids={} asks={}", ob.bids.size(), ob.asks.size());
    } catch (const std::exception& e) {
        Logger::get()->warn("[KuCoin] getOrderBook failed: {}", e.what());
    }
#else
    (void)symbol; (void)depth;
#endif
    return ob;
}

} // namespace crypto
