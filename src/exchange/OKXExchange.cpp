#include "OKXExchange.h"
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
size_t okxWriteCb(char* ptr, size_t size, size_t nmemb, std::string* data) {
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

OKXExchange::OKXExchange(const std::string& apiKey,
                           const std::string& apiSecret,
                           const std::string& passphrase,
                           const std::string& baseUrl,
                           const std::string& wsHost,
                           const std::string& wsPort)
    : apiKey_(apiKey), apiSecret_(apiSecret), passphrase_(passphrase),
      baseUrl_(baseUrl), wsHost_(wsHost), wsPort_(wsPort) {}

OKXExchange::~OKXExchange() { disconnect(); }

std::string OKXExchange::sign(const std::string& timestamp,
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

void OKXExchange::rateLimit() const {
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

std::string OKXExchange::httpGet(const std::string& path, bool signed_) const {
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
            // OKX expects ISO 8601 timestamp with milliseconds
            std::time_t secs = static_cast<std::time_t>(ts / 1000);
            int millis = static_cast<int>(ts % 1000);
            std::tm tm{};
#ifdef _WIN32
            gmtime_s(&tm, &secs);
#else
            gmtime_r(&secs, &tm);
#endif
            char tsBuf[32];
            std::strftime(tsBuf, sizeof(tsBuf), "%Y-%m-%dT%H:%M:%S", &tm);
            std::string timestamp = std::string(tsBuf) + "." +
                std::to_string(millis) + "Z";

            std::string sig = sign(timestamp, "GET", path);
            headers = curl_slist_append(headers, ("OK-ACCESS-KEY: " + apiKey_).c_str());
            headers = curl_slist_append(headers, ("OK-ACCESS-SIGN: " + sig).c_str());
            headers = curl_slist_append(headers, ("OK-ACCESS-TIMESTAMP: " + timestamp).c_str());
            headers = curl_slist_append(headers, ("OK-ACCESS-PASSPHRASE: " + passphrase_).c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (headers)
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, okxWriteCb);
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

std::vector<Candle> OKXExchange::getKlines(const std::string& symbol,
                                             const std::string& interval,
                                             int limit) {
    // OKX API max limit is 300 per request
    if (limit > 300) {
        Logger::get()->debug("[OKX] getKlines limit {} clamped to 300", limit);
        limit = 300;
    }
    Logger::get()->debug("[OKX] getKlines symbol={} interval={} limit={}", symbol, interval, limit);
    std::string path = "/api/v5/market/candles?instId=" + symbol +
                       "&bar=" + interval +
                       "&limit=" + std::to_string(limit);
    auto response = httpGet(path);
    Logger::get()->debug("[OKX] getKlines response size={}", response.size());
    auto json = nlohmann::json::parse(response);
    std::vector<Candle> candles;
    if (json["code"].get<std::string>() != "0") {
        Logger::get()->warn("[OKX] getKlines API error: code={} msg={}",
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
    // OKX API returns candles newest-first; reverse to chronological order
    std::reverse(candles.begin(), candles.end());
    Logger::get()->debug("[OKX] getKlines parsed {} candles", candles.size());
    return candles;
}

double OKXExchange::getPrice(const std::string& symbol) {
    Logger::get()->debug("[OKX] getPrice symbol={}", symbol);
    auto json = nlohmann::json::parse(
        httpGet("/api/v5/market/ticker?instId=" + symbol));
    if (json["code"].get<std::string>() != "0") {
        Logger::get()->warn("[OKX] getPrice API error: code={}", json["code"].get<std::string>());
        return 0.0;
    }
    double price = safeStod(json["data"][0]["last"].get<std::string>());
    Logger::get()->debug("[OKX] getPrice result={}", price);
    return price;
}

OrderResponse OKXExchange::placeOrder(const OrderRequest& req) {
    // Simplified — real implementation would POST to /api/v5/trade/order
    (void)req;
    return {};
}

AccountBalance OKXExchange::getBalance() {
    return {};
}

void OKXExchange::onWsMessage(const std::string& msg) {
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
                c.closed   = (k[8].get<std::string>() == "1");
                klineCb_(c);
            }
        }
    } catch (const std::exception& e) {
        Logger::get()->warn("OKX WS parse: {}", e.what());
    }
}

void OKXExchange::subscribeKline(const std::string& symbol,
                                  const std::string& interval,
                                  std::function<void(const Candle&)> cb) {
    klineCb_ = std::move(cb);
    ws_ = std::make_unique<WebSocketClient>(wsHost_, wsPort_, "/ws/v5/public");
    ws_->setMessageCallback([this](const std::string& m){ onWsMessage(m); });
    (void)symbol; (void)interval;
}

void OKXExchange::connect() {
    Logger::get()->debug("[OKX] Connecting WebSocket to {}:{}", wsHost_, wsPort_);
    if (ws_) ws_->connect();
}
void OKXExchange::disconnect() {
    Logger::get()->debug("[OKX] Disconnecting");
    if (ws_) ws_->disconnect();
}

bool OKXExchange::testConnection(std::string& outError) {
    Logger::get()->debug("[OKX] Testing connection to {}", baseUrl_);
    try {
        double price = getPrice("BTC-USDT");
        if (price > 0) {
            Logger::get()->info("[OKX] Connection test OK, BTC-USDT={}", price);
            return true;
        }
        outError = "Received invalid price (0) from OKX API";
        Logger::get()->warn("[OKX] Connection test failed: {}", outError);
        return false;
    } catch (const std::exception& e) {
        outError = std::string("OKX API error: ") + e.what();
        Logger::get()->warn("[OKX] Connection test failed: {}", outError);
        return false;
    }
}

void OKXExchange::setMarketType(const std::string& marketType) {
    marketType_ = marketType;
    Logger::get()->debug("[OKX] Market type set to: {}", marketType_);
}

std::vector<SymbolInfo> OKXExchange::getSymbols(const std::string& marketType) {
#ifndef USE_CURL
    (void)marketType;
    return {};
#else
    std::vector<SymbolInfo> result;
    try {
        // Spot instruments
        if (marketType.empty() || marketType == "spot") {
            auto resp = httpGet("/api/v5/public/instruments?instType=SPOT");
            auto j = nlohmann::json::parse(resp);
            if (j.contains("data") && j["data"].is_array()) {
                for (auto& s : j["data"]) {
                    if (s.value("state", "") != "live") continue;
                    SymbolInfo si;
                    si.symbol     = s.value("instId", "");
                    si.baseAsset  = s.value("baseCcy", "");
                    si.quoteAsset = s.value("quoteCcy", "");
                    si.marketType = "spot";
                    si.displayName = si.baseAsset + "/" + si.quoteAsset;
                    result.push_back(std::move(si));
                }
            }
        }
        // Futures (SWAP) instruments
        if (marketType.empty() || marketType == "futures") {
            auto resp = httpGet("/api/v5/public/instruments?instType=SWAP");
            auto j = nlohmann::json::parse(resp);
            if (j.contains("data") && j["data"].is_array()) {
                for (auto& s : j["data"]) {
                    if (s.value("state", "") != "live") continue;
                    SymbolInfo si;
                    si.symbol     = s.value("instId", "");
                    si.baseAsset  = s.value("ctValCcy", "");
                    si.quoteAsset = s.value("settleCcy", "");
                    si.marketType = "futures";
                    si.displayName = si.baseAsset + "/" + si.quoteAsset + " Swap";
                    result.push_back(std::move(si));
                }
            }
        }
        Logger::get()->info("[OKX] Fetched {} symbols", result.size());
    } catch (const std::exception& e) {
        Logger::get()->warn("[OKX] getSymbols failed: {}", e.what());
    }
    return result;
#endif
}

OrderBook OKXExchange::getOrderBook(const std::string& symbol, int depth) {
    OrderBook ob;
#ifdef USE_CURL
    try {
        Logger::get()->debug("[OKX] getOrderBook symbol={} depth={}", symbol, depth);
        std::string path = "/api/v5/market/books?instId=" + symbol +
                           "&sz=" + std::to_string(depth);
        auto response = httpGet(path);
        auto json = nlohmann::json::parse(response);
        if (json["code"].get<std::string>() != "0") {
            Logger::get()->warn("[OKX] getOrderBook API error: code={}", json["code"].get<std::string>());
            return ob;
        }
        auto& data = json["data"][0];
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
        Logger::get()->debug("[OKX] getOrderBook bids={} asks={}", ob.bids.size(), ob.asks.size());
    } catch (const std::exception& e) {
        Logger::get()->warn("[OKX] getOrderBook failed: {}", e.what());
    }
#else
    (void)symbol; (void)depth;
#endif
    return ob;
}

std::vector<OpenOrderInfo> OKXExchange::getOpenOrders(const std::string& symbol) {
#ifndef USE_CURL
    (void)symbol;
    return {};
#else
    try {
        std::string path = "/api/v5/trade/orders-pending";
        if (!symbol.empty()) path += "?instId=" + symbol;
        auto resp = httpGet(path, true);
        auto j = nlohmann::json::parse(resp);
        std::vector<OpenOrderInfo> result;
        if (j.contains("data") && j["data"].is_array()) {
            for (auto& o : j["data"]) {
                OpenOrderInfo info;
                info.orderId = o.value("ordId", "");
                info.symbol  = o.value("instId", "");
                info.side    = o.value("side", "");
                // Uppercase side for consistency
                for (auto& ch : info.side) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                info.type    = o.value("ordType", "");
                info.price   = safeStod(o.value("px", "0"));
                info.qty     = safeStod(o.value("sz", "0"));
                info.executedQty = safeStod(o.value("fillSz", "0"));
                info.time    = std::stoll(o.value("cTime", "0"));
                result.push_back(std::move(info));
            }
        }
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[OKX] getOpenOrders failed: {}", e.what());
        return {};
    }
#endif
}

std::vector<UserTradeInfo> OKXExchange::getMyTrades(const std::string& symbol, int limit) {
#ifndef USE_CURL
    (void)symbol; (void)limit;
    return {};
#else
    try {
        std::string path = "/api/v5/trade/fills?instId=" + symbol +
                           "&limit=" + std::to_string(limit);
        auto resp = httpGet(path, true);
        auto j = nlohmann::json::parse(resp);
        std::vector<UserTradeInfo> result;
        if (j.contains("data") && j["data"].is_array()) {
            for (auto& t : j["data"]) {
                UserTradeInfo info;
                info.time    = std::stoll(t.value("ts", "0"));
                info.symbol  = t.value("instId", "");
                info.side    = t.value("side", "");
                for (auto& ch : info.side) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                info.price   = safeStod(t.value("fillPx", "0"));
                info.qty     = safeStod(t.value("fillSz", "0"));
                info.commission     = safeStod(t.value("fee", "0"));
                info.commissionAsset = t.value("feeCcy", "");
                result.push_back(std::move(info));
            }
        }
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[OKX] getMyTrades failed: {}", e.what());
        return {};
    }
#endif
}

std::vector<PositionInfo> OKXExchange::getPositionRisk(const std::string& symbol) {
#ifndef USE_CURL
    (void)symbol;
    return {};
#else
    try {
        std::string path = "/api/v5/account/positions";
        if (!symbol.empty()) path += "?instId=" + symbol;
        auto resp = httpGet(path, true);
        auto j = nlohmann::json::parse(resp);
        std::vector<PositionInfo> result;
        if (j.contains("data") && j["data"].is_array()) {
            for (auto& p : j["data"]) {
                PositionInfo info;
                info.symbol           = p.value("instId", "");
                info.positionAmt      = safeStod(p.value("pos", "0"));
                info.entryPrice       = safeStod(p.value("avgPx", "0"));
                info.unrealizedProfit = safeStod(p.value("upl", "0"));
                info.realizedProfit   = safeStod(p.value("realizedPnl", "0"));
                info.marginType       = p.value("mgnMode", "");
                info.leverage         = safeStod(p.value("lever", "1"));
                result.push_back(std::move(info));
            }
        }
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[OKX] getPositionRisk failed: {}", e.what());
        return {};
    }
#endif
}

std::vector<AccountBalanceDetail> OKXExchange::getAccountBalanceDetails() {
#ifndef USE_CURL
    return {};
#else
    try {
        auto resp = httpGet("/api/v5/account/balance", true);
        auto j = nlohmann::json::parse(resp);
        std::vector<AccountBalanceDetail> result;
        if (j.contains("data") && j["data"].is_array() && !j["data"].empty()) {
            auto& details = j["data"][0]["details"];
            if (details.is_array()) {
                for (auto& d : details) {
                    AccountBalanceDetail info;
                    info.currency  = d.value("ccy", "");
                    info.availBal  = safeStod(d.value("availBal", "0"));
                    info.frozenBal = safeStod(d.value("frozenBal", "0"));
                    info.usdValue  = safeStod(d.value("eqUsd", "0"));
                    result.push_back(std::move(info));
                }
            }
        }
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[OKX] getAccountBalanceDetails failed: {}", e.what());
        return {};
    }
#endif
}

} // namespace crypto
