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
#include <thread>
#include <algorithm>
#include <stdexcept>

namespace crypto {

namespace {
inline double safeStod(const std::string& s, double defaultVal = 0.0) {
    if (s.empty()) return defaultVal;
    try { return std::stod(s); } catch (...) { return defaultVal; }
}
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

void BybitExchange::rateLimit() const {
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

std::string BybitExchange::httpGet(const std::string& path) const {
#ifndef USE_CURL
    throw std::runtime_error("libcurl not available");
    (void)path;
#else
    rateLimit();
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
    // Bybit API max limit is 1000
    if (limit > 1000) {
        Logger::get()->debug("[Bybit] getKlines limit {} clamped to 1000", limit);
        limit = 1000;
    }
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
        c.open     = safeStod(k[1].get<std::string>());
        c.high     = safeStod(k[2].get<std::string>());
        c.low      = safeStod(k[3].get<std::string>());
        c.close    = safeStod(k[4].get<std::string>());
        c.volume   = safeStod(k[5].get<std::string>());
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
    if (!json.contains("result") || !json["result"].contains("list") ||
        !json["result"]["list"].is_array() || json["result"]["list"].empty()) {
        Logger::get()->warn("[Bybit] getPrice: empty result list");
        return 0.0;
    }
    double price = safeStod(json["result"]["list"][0]["lastPrice"].get<std::string>());
    Logger::get()->debug("[Bybit] getPrice result={}", price);
    return price;
}

OrderResponse BybitExchange::placeOrder(const OrderRequest& req) {
#ifndef USE_CURL
    (void)req;
    return {};
#else
    try {
        rateLimit();
        std::string side = (req.side == OrderRequest::Side::BUY) ? "Buy" : "Sell";
        std::string orderType;
        switch (req.type) {
            case OrderRequest::Type::MARKET:    orderType = "Market"; break;
            case OrderRequest::Type::LIMIT:     orderType = "Limit"; break;
            case OrderRequest::Type::STOP_LOSS: orderType = "Market"; break;
        }

        nlohmann::json body;
        body["category"] = "spot";
        body["symbol"] = req.symbol;
        body["side"] = side;
        body["orderType"] = orderType;
        body["qty"] = std::to_string(req.qty);
        if (req.type == OrderRequest::Type::LIMIT && req.price > 0)
            body["price"] = std::to_string(req.price);

        auto resp = httpPost("/v5/order/create", body.dump());
        auto j = nlohmann::json::parse(resp);

        OrderResponse result;
        if (j.value("retCode", -1) != 0) {
            Logger::get()->warn("[Bybit] placeOrder error: retCode={} msg={}",
                                j.value("retCode", -1), j.value("retMsg", "unknown"));
            result.status = "ERROR";
            return result;
        }
        if (j.contains("result")) {
            result.orderId = j["result"].value("orderId", "");
        }
        result.status = "NEW";
        result.executedQty = req.qty;
        result.executedPrice = req.price;
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bybit] placeOrder failed: {}", e.what());
        OrderResponse result;
        result.status = "ERROR";
        return result;
    }
#endif
}

std::string BybitExchange::httpPost(const std::string& path,
                                     const std::string& body) const {
#ifndef USE_CURL
    throw std::runtime_error("libcurl not available");
    (void)path; (void)body;
#else
    rateLimit();
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string timestamp = std::to_string(ts);
    std::string recvWindow = "5000";

    // Bybit V5 sign: timestamp + apiKey + recvWindow + body
    std::string payload = timestamp + apiKey_ + recvWindow + body;
    std::string sig = sign(payload);

    std::string url = baseUrl_ + path;
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-BAPI-API-KEY: " + apiKey_).c_str());
    headers = curl_slist_append(headers, ("X-BAPI-SIGN: " + sig).c_str());
    headers = curl_slist_append(headers, ("X-BAPI-TIMESTAMP: " + timestamp).c_str());
    headers = curl_slist_append(headers, ("X-BAPI-RECV-WINDOW: " + recvWindow).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, bybitWriteCb);
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

AccountBalance BybitExchange::getBalance() {
#ifndef USE_CURL
    return {};
#else
    try {
        auto resp = httpGet("/v5/account/wallet-balance?accountType=UNIFIED");
        auto j = nlohmann::json::parse(resp);
        AccountBalance bal;
        if (j.value("retCode", -1) != 0) {
            Logger::get()->warn("[Bybit] getBalance API error: {}",
                                j.value("retMsg", "unknown"));
            return bal;
        }
        if (j.contains("result") && j["result"].contains("list") &&
            j["result"]["list"].is_array() && !j["result"]["list"].empty()) {
            auto& coins = j["result"]["list"][0]["coin"];
            if (coins.is_array()) {
                for (auto& c : coins) {
                    std::string coin = c.value("coin", "");
                    if (coin == "USDT") {
                        bal.totalUSDT     = safeStod(c.value("walletBalance", "0"));
                        bal.availableUSDT = safeStod(c.value("availableToWithdraw", "0"));
                    }
                    if (coin == "BTC") {
                        bal.btcBalance = safeStod(c.value("walletBalance", "0"));
                    }
                }
            }
        }
        return bal;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bybit] getBalance failed: {}", e.what());
        return {};
    }
#endif
}

void BybitExchange::onWsMessage(const std::string& msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        if (!j.contains("data")) return;
        if (klineCb_) {
            auto& d = j["data"][0];
            Candle c;
            c.openTime = std::stoll(d["start"].get<std::string>());
            c.open     = safeStod(d["open"].get<std::string>());
            c.high     = safeStod(d["high"].get<std::string>());
            c.low      = safeStod(d["low"].get<std::string>());
            c.close    = safeStod(d["close"].get<std::string>());
            c.volume   = safeStod(d["volume"].get<std::string>());
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
    ws_->connect();
    // Send subscription message
    nlohmann::json sub;
    sub["op"] = "subscribe";
    sub["args"] = nlohmann::json::array({"kline." + interval + "." + symbol});
    ws_->send(sub.dump());
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

std::vector<SymbolInfo> BybitExchange::getSymbols(const std::string& marketType) {
#ifndef USE_CURL
    (void)marketType;
    return {};
#else
    std::vector<SymbolInfo> result;
    try {
        // Fetch spot symbols
        if (marketType.empty() || marketType == "spot") {
            auto resp = httpGet("/v5/market/instruments-info?category=spot");
            auto j = nlohmann::json::parse(resp);
            if (j.contains("result") && j["result"].contains("list")) {
                for (auto& s : j["result"]["list"]) {
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
        // Fetch futures (linear) symbols
        if (marketType.empty() || marketType == "futures") {
            auto resp = httpGet("/v5/market/instruments-info?category=linear");
            auto j = nlohmann::json::parse(resp);
            if (j.contains("result") && j["result"].contains("list")) {
                for (auto& s : j["result"]["list"]) {
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
        Logger::get()->info("[Bybit] Fetched {} symbols", result.size());
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bybit] getSymbols failed: {}", e.what());
    }
    return result;
#endif
}

OrderBook BybitExchange::getOrderBook(const std::string& symbol, int depth) {
    OrderBook ob;
#ifdef USE_CURL
    try {
        Logger::get()->debug("[Bybit] getOrderBook symbol={} depth={}", symbol, depth);
        std::string path = "/v5/market/orderbook?category=linear&symbol=" + symbol +
                           "&limit=" + std::to_string(depth);
        auto response = httpGet(path);
        auto json = nlohmann::json::parse(response);
        if (json["retCode"].get<int>() != 0) {
            Logger::get()->warn("[Bybit] getOrderBook API error: retCode={}", json["retCode"].get<int>());
            return ob;
        }
        auto& result = json["result"];
        for (auto& b : result["b"]) {
            OrderBook::Level lvl;
            lvl.price = safeStod(b[0].get<std::string>());
            lvl.qty   = safeStod(b[1].get<std::string>());
            ob.bids.push_back(lvl);
        }
        for (auto& a : result["a"]) {
            OrderBook::Level lvl;
            lvl.price = safeStod(a[0].get<std::string>());
            lvl.qty   = safeStod(a[1].get<std::string>());
            ob.asks.push_back(lvl);
        }
        Logger::get()->debug("[Bybit] getOrderBook bids={} asks={}", ob.bids.size(), ob.asks.size());
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bybit] getOrderBook failed: {}", e.what());
    }
#else
    (void)symbol; (void)depth;
#endif
    return ob;
}

std::vector<PositionInfo> BybitExchange::getPositionRisk(const std::string& symbol) {
#ifndef USE_CURL
    (void)symbol;
    return {};
#else
    try {
        rateLimit();
        std::string path = "/v5/position/list?category=linear";
        if (!symbol.empty()) path += "&symbol=" + symbol;
        auto resp = httpGet(path);
        auto j = nlohmann::json::parse(resp);
        std::vector<PositionInfo> result;
        if (j.contains("result") && j["result"].contains("list") && j["result"]["list"].is_array()) {
            for (auto& p : j["result"]["list"]) {
                PositionInfo info;
                info.symbol           = p.value("symbol", "");
                info.positionAmt      = safeStod(p.value("size", "0"));
                info.entryPrice       = safeStod(p.value("avgPrice", "0"));
                info.unrealizedProfit = safeStod(p.value("unrealisedPnl", "0"));
                info.realizedProfit   = safeStod(p.value("cumRealisedPnl", "0"));
                info.marginType       = p.value("tradeMode", "");
                info.leverage         = safeStod(p.value("leverage", "1"));
                result.push_back(std::move(info));
            }
        }
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bybit] getPositionRisk failed: {}", e.what());
        return {};
    }
#endif
}

bool BybitExchange::cancelOrder(const std::string& symbol, const std::string& orderId) {
#ifndef USE_CURL
    (void)symbol; (void)orderId;
    return false;
#else
    try {
        rateLimit();
        nlohmann::json body;
        body["category"] = "spot";
        body["symbol"]   = symbol;
        body["orderId"]  = orderId;
        auto resp = httpPost("/v5/order/cancel", body.dump());
        auto j = nlohmann::json::parse(resp);
        if (j.value("retCode", -1) != 0) {
            Logger::get()->warn("[Bybit] cancelOrder error: {}", j.value("retMsg", ""));
            return false;
        }
        Logger::get()->info("[Bybit] cancelOrder success: {} {}", symbol, orderId);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bybit] cancelOrder failed: {}", e.what());
        return false;
    }
#endif
}

bool BybitExchange::setLeverage(const std::string& symbol, int leverage) {
#ifndef USE_CURL
    (void)symbol; (void)leverage;
    return false;
#else
    try {
        rateLimit();
        nlohmann::json body;
        body["category"]     = "linear";
        body["symbol"]       = symbol;
        body["buyLeverage"]  = std::to_string(leverage);
        body["sellLeverage"] = std::to_string(leverage);
        auto resp = httpPost("/v5/position/set-leverage", body.dump());
        auto j = nlohmann::json::parse(resp);
        int code = j.value("retCode", -1);
        if (code != 0 && code != 110043) { // 110043 = leverage not modified
            Logger::get()->warn("[Bybit] setLeverage error: {}", j.value("retMsg", ""));
            return false;
        }
        Logger::get()->info("[Bybit] setLeverage: {} = {}x", symbol, leverage);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bybit] setLeverage failed: {}", e.what());
        return false;
    }
#endif
}

int BybitExchange::getLeverage(const std::string& symbol) {
#ifndef USE_CURL
    (void)symbol;
    return 1;
#else
    try {
        auto positions = getPositionRisk(symbol);
        for (auto& p : positions) {
            if (p.symbol == symbol && p.leverage > 0)
                return static_cast<int>(p.leverage);
        }
        return 1;
    } catch (...) {
        return 1;
    }
#endif
}

FuturesBalanceInfo BybitExchange::getFuturesBalance() {
#ifndef USE_CURL
    return {};
#else
    try {
        rateLimit();
        std::string path = "/v5/account/wallet-balance?accountType=CONTRACT";
        auto resp = httpGet(path);
        auto j = nlohmann::json::parse(resp);

        if (j.value("retCode", -1) != 0) {
            Logger::get()->warn("[Bybit] getFuturesBalance error: {}", j.value("retMsg", ""));
            return {};
        }

        FuturesBalanceInfo info;
        if (j.contains("result") && j["result"].contains("list") && j["result"]["list"].is_array()) {
            for (auto& acct : j["result"]["list"]) {
                info.totalWalletBalance    = safeStod(acct.value("totalWalletBalance", "0"));
                info.totalUnrealizedProfit = safeStod(acct.value("totalPerpUPL", "0"));
                info.totalMarginBalance    = safeStod(acct.value("totalMarginBalance", "0"));
                if (acct.contains("coin") && acct["coin"].is_array()) {
                    for (auto& c : acct["coin"]) {
                        if (c.value("coin", "") == "USDT") {
                            info.totalWalletBalance    = safeStod(c.value("walletBalance", "0"));
                            info.totalUnrealizedProfit = safeStod(c.value("unrealisedPnl", "0"));
                            info.totalMarginBalance    = safeStod(c.value("equity", "0"));
                            break;
                        }
                    }
                }
                break; // first account
            }
        }
        return info;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Bybit] getFuturesBalance failed: {}", e.what());
        return {};
    }
#endif
}

} // namespace crypto
