#include "BinanceExchange.h"
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
#include <stdexcept>

namespace crypto {

namespace {
// Safe string-to-double: returns defaultVal for empty or non-numeric strings
inline double safeStod(const std::string& s, double defaultVal = 0.0) {
    if (s.empty()) return defaultVal;
    try {
        return std::stod(s);
    } catch (...) {
        return defaultVal;
    }
}
} // namespace

#ifdef USE_CURL
namespace {
size_t writeCallback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string toHex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    return oss.str();
}
} // namespace
#else
namespace {
std::string toHex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    return oss.str();
}
} // namespace
#endif

BinanceExchange::BinanceExchange(const std::string& apiKey,
                                   const std::string& apiSecret,
                                   const std::string& baseUrl,
                                   const std::string& wsHost,
                                   const std::string& wsPort,
                                   const std::string& futuresBaseUrl,
                                   const std::string& futuresWsHost,
                                   const std::string& futuresWsPort)
    : apiKey_(apiKey), apiSecret_(apiSecret),
      baseUrl_(baseUrl), wsHost_(wsHost), wsPort_(wsPort)
{
    // Detect testnet by checking the base URL and set futures URLs accordingly
    bool isTestnet = (baseUrl_.find("testnet") != std::string::npos);
    if (!futuresBaseUrl.empty()) {
        futuresBaseUrl_ = futuresBaseUrl;
    } else {
        futuresBaseUrl_ = isTestnet ? "https://testnet.binancefuture.com"
                                    : "https://fapi.binance.com";
    }
    if (!futuresWsHost.empty()) {
        futuresWsHost_ = futuresWsHost;
    } else {
        futuresWsHost_ = isTestnet ? "fstream.binancefuture.com"
                                   : "fstream.binance.com";
    }
    if (!futuresWsPort.empty()) {
        futuresWsPort_ = futuresWsPort;
    } else {
        futuresWsPort_ = isTestnet ? "443" : "9443";
    }
}

BinanceExchange::~BinanceExchange() {
    disconnect();
}

std::string BinanceExchange::sign(const std::string& payload) const {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    HMAC(EVP_sha256(),
         apiSecret_.c_str(), static_cast<int>(apiSecret_.size()),
         reinterpret_cast<const unsigned char*>(payload.c_str()),
         payload.size(), digest, &digestLen);
    return toHex(digest, digestLen);
}

std::string BinanceExchange::effectiveBaseUrl() const {
    return (marketType_ == "futures") ? futuresBaseUrl_ : baseUrl_;
}

std::string BinanceExchange::replaceBase(const std::string& url,
                                          const std::string& oldBase,
                                          const std::string& newBase) const {
    if (url.find(oldBase) == 0) {
        return newBase + url.substr(oldBase.size());
    }
    return url;
}

void BinanceExchange::setMarketType(const std::string& marketType) {
    marketType_ = marketType;
}

void BinanceExchange::rateLimit() const {
    std::lock_guard<std::mutex> lock(rateMutex_);
    auto now = std::chrono::steady_clock::now();
    auto windowStart = now - std::chrono::minutes(1);

    // Purge entries older than 1 minute
    while (!requestTimes_.empty() && requestTimes_.front() < windowStart) {
        requestTimes_.pop();
    }

    int maxPerMinute = (marketType_ == "futures")
        ? MAX_REQUESTS_PER_MINUTE_FUTURES
        : MAX_REQUESTS_PER_MINUTE_SPOT;

    if (static_cast<int>(requestTimes_.size()) >= maxPerMinute) {
        auto sleepUntil = requestTimes_.front() + std::chrono::minutes(1);
        std::this_thread::sleep_until(sleepUntil);
        // Purge again after sleeping
        auto currentTime = std::chrono::steady_clock::now();
        auto updatedWindowStart = currentTime - std::chrono::minutes(1);
        while (!requestTimes_.empty() && requestTimes_.front() < updatedWindowStart) {
            requestTimes_.pop();
        }
    }
    requestTimes_.push(std::chrono::steady_clock::now());
}

void BinanceExchange::orderRateLimit() const {
    std::lock_guard<std::mutex> lock(orderRateMutex_);
    auto now = std::chrono::steady_clock::now();

    // Reset daily counter every 24 hours
    if (now - dailyOrderReset_ >= std::chrono::hours(24)) {
        dailyOrderCount_ = 0;
        dailyOrderReset_ = now;
    }

    if (marketType_ == "futures") {
        // Futures: 1200 orders per minute
        auto windowStart = now - std::chrono::minutes(1);
        while (!orderTimes_.empty() && orderTimes_.front() < windowStart) {
            orderTimes_.pop();
        }
        if (static_cast<int>(orderTimes_.size()) >= MAX_ORDERS_PER_MINUTE_FUTURES) {
            auto sleepUntil = orderTimes_.front() + std::chrono::minutes(1);
            std::this_thread::sleep_until(sleepUntil);
        }
    } else {
        // Spot: 100 orders per 10 seconds
        auto windowStart = now - std::chrono::seconds(10);
        while (!orderTimes_.empty() && orderTimes_.front() < windowStart) {
            orderTimes_.pop();
        }
        if (static_cast<int>(orderTimes_.size()) >= MAX_ORDERS_PER_10S_SPOT) {
            auto sleepUntil = orderTimes_.front() + std::chrono::seconds(10);
            std::this_thread::sleep_until(sleepUntil);
        }
        // Spot: 200 000 orders per 24 hours
        if (dailyOrderCount_.load() >= MAX_ORDERS_PER_24H_SPOT) {
            Logger::get()->warn("[Binance] Daily order limit ({}) reached, waiting for reset",
                                MAX_ORDERS_PER_24H_SPOT);
            std::this_thread::sleep_until(dailyOrderReset_ + std::chrono::hours(24));
            dailyOrderCount_ = 0;
            dailyOrderReset_ = std::chrono::steady_clock::now();
        }
    }
    orderTimes_.push(std::chrono::steady_clock::now());
    dailyOrderCount_.fetch_add(1);
}

void BinanceExchange::wsConnectionRateLimit() const {
    std::lock_guard<std::mutex> lock(wsRateMutex_);
    auto now = std::chrono::steady_clock::now();
    auto windowStart = now - std::chrono::minutes(5);

    while (!wsConnectTimes_.empty() && wsConnectTimes_.front() < windowStart) {
        wsConnectTimes_.pop();
    }
    if (static_cast<int>(wsConnectTimes_.size()) >= MAX_WS_CONNECTIONS_PER_5MIN) {
        auto sleepUntil = wsConnectTimes_.front() + std::chrono::minutes(5);
        Logger::get()->warn("[Binance] WS connection rate limit ({}/5min) reached, throttling",
                            MAX_WS_CONNECTIONS_PER_5MIN);
        std::this_thread::sleep_until(sleepUntil);
    }
    wsConnectTimes_.push(std::chrono::steady_clock::now());
}

std::string BinanceExchange::httpGet(const std::string& path, bool signed_) const {
#ifndef USE_CURL
    throw std::runtime_error("libcurl not available");
    (void)path; (void)signed_;
#else
    rateLimit();
    std::string base = effectiveBaseUrl();
    std::string url = base + path;
    if (signed_) {
        auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto qpos = path.find('?');
        std::string queryString;
        if (qpos != std::string::npos) {
            queryString = path.substr(qpos + 1) + "&timestamp=" + std::to_string(ts);
        } else {
            queryString = "timestamp=" + std::to_string(ts);
        }
        std::string sig = sign(queryString);
        url = base + path +
              (qpos == std::string::npos ? "?" : "&") +
              "timestamp=" + std::to_string(ts) + "&signature=" + sig;
    }

    int retries = 3;
    while (retries-- > 0) {
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl_easy_init failed");

        std::string response;
        struct curl_slist* headers = nullptr;
        if (!apiKey_.empty())
            headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + apiKey_).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK && httpCode < 400) {
            useAltEndpoint_ = (url.find(base) != 0);
            return response;
        }

        // Geo-blocking fallback: try alternative endpoint on 403/451
        if (ExchangeEndpointManager::isGeoBlocked(static_cast<int>(httpCode))) {
            bool isFutures = (marketType_ == "futures");
            const auto& endpoints = isFutures
                ? ExchangeEndpointManager::binanceFuturesEndpoints()
                : ExchangeEndpointManager::binanceEndpoints();
            int& idx = isFutures ? futuresEndpointIdx_ : spotEndpointIdx_;
            std::string alt = ExchangeEndpointManager::getNextEndpoint(endpoints, idx);
            if (!alt.empty()) {
                url = replaceBase(url, base, alt);
                base = alt;
                useAltEndpoint_ = true;
                Logger::get()->warn("[Binance] Geo-blocked (HTTP {}), trying: {}",
                                    httpCode, alt);
            }
            continue;
        }

        if (httpCode == 429 || httpCode == 418) {
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

std::string BinanceExchange::httpGetUrl(const std::string& fullUrl, bool signed_) const {
#ifndef USE_CURL
    throw std::runtime_error("libcurl not available");
    (void)fullUrl; (void)signed_;
#else
    rateLimit();
    std::string url = fullUrl;
    if (signed_) {
        auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto qpos = fullUrl.find('?');
        std::string queryString;
        if (qpos != std::string::npos) {
            queryString = fullUrl.substr(qpos + 1) + "&timestamp=" + std::to_string(ts);
        } else {
            queryString = "timestamp=" + std::to_string(ts);
        }
        std::string sig = sign(queryString);
        url = fullUrl +
              (qpos == std::string::npos ? "?" : "&") +
              "timestamp=" + std::to_string(ts) + "&signature=" + sig;
    }

    int retries = 3;
    while (retries-- > 0) {
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl_easy_init failed");

        std::string response;
        struct curl_slist* headers = nullptr;
        if (!apiKey_.empty())
            headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + apiKey_).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK && httpCode < 400) return response;

        if (httpCode == 429 || httpCode == 418) {
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

std::string BinanceExchange::httpPost(const std::string& path,
                                       const std::string& body) const {
#ifndef USE_CURL
    throw std::runtime_error("libcurl not available");
    (void)path; (void)body;
#else
    rateLimit();
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string signedBody = body + "&timestamp=" + std::to_string(ts);
    std::string sig = sign(signedBody);
    std::string fullBody = signedBody + "&signature=" + sig;

    std::string url = effectiveBaseUrl() + path;
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + apiKey_).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fullBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

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

Candle BinanceExchange::parseKlineJson(const nlohmann::json& k) const {
    Candle c;
    c.openTime  = k[0].get<int64_t>();
    c.open      = safeStod(k[1].get<std::string>());
    c.high      = safeStod(k[2].get<std::string>());
    c.low       = safeStod(k[3].get<std::string>());
    c.close     = safeStod(k[4].get<std::string>());
    c.volume    = safeStod(k[5].get<std::string>());
    c.closeTime = k[6].get<int64_t>();
    c.trades    = k[8].get<int64_t>();
    c.closed    = true;
    return c;
}

std::vector<Candle> BinanceExchange::getKlines(const std::string& symbol,
                                                 const std::string& interval,
                                                 int limit) {
    // Binance API enforces max limit: 1000 for spot, 1500 for futures
    const int maxLimit = (marketType_ == "futures") ? 1500 : 1000;
    if (limit > maxLimit) limit = maxLimit;
    Logger::get()->debug("[Binance] getKlines symbol={} interval={} limit={}", symbol, interval, limit);
    std::string path = (marketType_ == "futures" ? "/fapi/v1/klines?" : "/api/v3/klines?");
    path += "symbol=" + symbol +
            "&interval=" + interval +
            "&limit=" + std::to_string(limit);
    auto response = httpGet(path);
    Logger::get()->debug("[Binance] getKlines response size={}", response.size());
    auto json = nlohmann::json::parse(response);
    std::vector<Candle> candles;
    candles.reserve(json.size());
    for (auto& k : json) candles.push_back(parseKlineJson(k));
    Logger::get()->debug("[Binance] getKlines parsed {} candles", candles.size());
    return candles;
}

double BinanceExchange::getPrice(const std::string& symbol) {
    Logger::get()->debug("[Binance] getPrice symbol={}", symbol);
    std::string path = (marketType_ == "futures")
        ? "/fapi/v1/ticker/price?symbol=" + symbol
        : "/api/v3/ticker/price?symbol=" + symbol;
    auto json = nlohmann::json::parse(httpGet(path));
    double price = safeStod(json["price"].get<std::string>());
    Logger::get()->debug("[Binance] getPrice result={}", price);
    return price;
}

OrderResponse BinanceExchange::placeOrder(const OrderRequest& req) {
    orderRateLimit();
    std::ostringstream body;
    body << "symbol=" << req.symbol
         << "&side=" << (req.side == OrderRequest::Side::BUY ? "BUY" : "SELL")
         << "&type=";
    switch (req.type) {
        case OrderRequest::Type::MARKET:   body << "MARKET"; break;
        case OrderRequest::Type::LIMIT:    body << "LIMIT&timeInForce=GTC&price=" << req.price; break;
        case OrderRequest::Type::STOP_LOSS: body << "STOP_LOSS_LIMIT&stopPrice=" << req.stopPrice
                                                  << "&price=" << req.price
                                                  << "&timeInForce=GTC"; break;
    }
    body << "&quantity=" << req.qty;

    std::string orderPath = (marketType_ == "futures") ? "/fapi/v1/order" : "/api/v3/order";
    auto json = nlohmann::json::parse(httpPost(orderPath, body.str()));
    OrderResponse resp;
    resp.orderId      = json.contains("orderId") ? std::to_string(json["orderId"].get<long>()) : "";
    resp.status       = json.value("status", "UNKNOWN");
    resp.executedQty  = safeStod(json.value("executedQty", "0"));
    resp.executedPrice= safeStod(json.value("price", "0"));
    return resp;
}

AccountBalance BinanceExchange::getBalance() {
    if (marketType_ == "futures") {
        // Use futures account endpoint for correct balance retrieval
        auto fInfo = getFuturesBalance();
        AccountBalance bal;
        bal.totalUSDT     = fInfo.totalWalletBalance;
        bal.availableUSDT = fInfo.totalMarginBalance;
        return bal;
    }
    auto json = nlohmann::json::parse(httpGet("/api/v3/account", true));
    AccountBalance bal;
    for (auto& asset : json["balances"]) {
        std::string a = asset["asset"].get<std::string>();
        double free = safeStod(asset["free"].get<std::string>());
        if (a == "USDT") bal.availableUSDT = bal.totalUSDT = free;
        if (a == "BTC")  bal.btcBalance = free;
    }
    return bal;
}

void BinanceExchange::onWsMessage(const std::string& msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        if (!j.contains("k")) return;
        auto& k = j["k"];
        Candle c;
        c.openTime  = k["t"].get<int64_t>();
        c.closeTime = k["T"].get<int64_t>();
        c.open      = safeStod(k["o"].get<std::string>());
        c.high      = safeStod(k["h"].get<std::string>());
        c.low       = safeStod(k["l"].get<std::string>());
        c.close     = safeStod(k["c"].get<std::string>());
        c.volume    = safeStod(k["v"].get<std::string>());
        c.trades    = k["n"].get<int64_t>();
        c.closed    = k["x"].get<bool>();
        if (klineCb_) klineCb_(c);
    } catch (const std::exception& e) {
        Logger::get()->warn("WS parse error: {}", e.what());
    }
}

void BinanceExchange::subscribeKline(const std::string& symbol,
                                      const std::string& interval,
                                      std::function<void(const Candle&)> cb) {
    wsConnectionRateLimit();
    klineCb_ = std::move(cb);
    // NOTE: Use individual symbol streams only.
    // Aggregate streams like !ticker@arr are deprecated (removed March 26, 2026).
    std::string path = "/stream?streams=" +
        [&]{ std::string s = symbol; for(auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; }() +
        "@kline_" + interval;
    if (marketType_ == "futures") {
        ws_ = std::make_unique<WebSocketClient>(futuresWsHost_, futuresWsPort_, path);
    } else {
        ws_ = std::make_unique<WebSocketClient>(wsHost_, wsPort_, path);
    }
    ws_->setMessageCallback([this](const std::string& m){ onWsMessage(m); });
}

void BinanceExchange::connect() {
    Logger::get()->debug("[Binance] Connecting WebSocket to {}:{}", wsHost_, wsPort_);
    if (ws_) ws_->connect();
}

void BinanceExchange::disconnect() {
    Logger::get()->debug("[Binance] Disconnecting");
    if (ws_) ws_->disconnect();
}

bool BinanceExchange::testConnection(std::string& outError) {
    Logger::get()->debug("[Binance] Testing connection to {}", baseUrl_);
    try {
        double price = getPrice("BTCUSDT");
        if (price > 0) {
            Logger::get()->info("[Binance] Connection test OK, BTCUSDT={}", price);
            return true;
        }
        outError = "Received invalid price (0) from Binance API";
        Logger::get()->warn("[Binance] Connection test failed: {}", outError);
        return false;
    } catch (const std::exception& e) {
        outError = std::string("Binance API error: ") + e.what();
        Logger::get()->warn("[Binance] Connection test failed: {}", outError);
        return false;
    }
}

std::vector<SymbolInfo> BinanceExchange::getSymbols(const std::string& marketType) {
#ifndef USE_CURL
    (void)marketType;
    return {};
#else
    std::vector<SymbolInfo> result;
    try {
        // Fetch spot symbols if requested or no filter
        if (marketType.empty() || marketType == "spot") {
            auto resp = httpGetUrl(baseUrl_ + "/api/v3/exchangeInfo");
            auto j = nlohmann::json::parse(resp);
            if (j.contains("symbols") && j["symbols"].is_array()) {
                for (auto& s : j["symbols"]) {
                    if (s.value("status", "") != "TRADING") continue;
                    SymbolInfo si;
                    si.symbol     = s.value("symbol", "");
                    si.baseAsset  = s.value("baseAsset", "");
                    si.quoteAsset = s.value("quoteAsset", "");
                    si.marketType = "spot";
                    si.displayName = si.baseAsset + "/" + si.quoteAsset;
                    result.push_back(std::move(si));
                }
            }
        }
        // Fetch futures symbols if requested or no filter
        if (marketType.empty() || marketType == "futures") {
            auto resp = httpGetUrl(futuresBaseUrl_ + "/fapi/v1/exchangeInfo");
            auto j = nlohmann::json::parse(resp);
            if (j.contains("symbols") && j["symbols"].is_array()) {
                for (auto& s : j["symbols"]) {
                    if (s.value("status", "") != "TRADING") continue;
                    SymbolInfo si;
                    si.symbol     = s.value("symbol", "");
                    si.baseAsset  = s.value("baseAsset", "");
                    si.quoteAsset = s.value("quoteAsset", "");
                    si.marketType = "futures";
                    si.displayName = si.baseAsset + "/" + si.quoteAsset + " Swap";
                    result.push_back(std::move(si));
                }
            }
        }
        Logger::get()->info("[Binance] Fetched {} symbols", result.size());
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Binance] getSymbols failed: {}", e.what());
        return result;
    }
#endif
}

OrderBook BinanceExchange::getOrderBook(const std::string& symbol, int depth) {
    OrderBook ob;
#ifdef USE_CURL
    try {
        Logger::get()->debug("[Binance] getOrderBook symbol={} depth={}", symbol, depth);
        std::string path;
        if (marketType_ == "futures") {
            path = "/fapi/v1/depth?symbol=" + symbol +
                   "&limit=" + std::to_string(depth);
        } else {
            path = "/api/v3/depth?symbol=" + symbol +
                   "&limit=" + std::to_string(depth);
        }
        auto response = httpGet(path);
        auto json = nlohmann::json::parse(response);

        ob.lastUpdateId = json.value("lastUpdateId", (int64_t)0);
        for (auto& b : json["bids"]) {
            OrderBook::Level lvl;
            lvl.price = safeStod(b[0].get<std::string>());
            lvl.qty   = safeStod(b[1].get<std::string>());
            ob.bids.push_back(lvl);
        }
        for (auto& a : json["asks"]) {
            OrderBook::Level lvl;
            lvl.price = safeStod(a[0].get<std::string>());
            lvl.qty   = safeStod(a[1].get<std::string>());
            ob.asks.push_back(lvl);
        }
        Logger::get()->debug("[Binance] getOrderBook bids={} asks={}", ob.bids.size(), ob.asks.size());
    } catch (const std::exception& e) {
        Logger::get()->warn("[Binance] getOrderBook failed: {}", e.what());
    }
#else
    (void)symbol; (void)depth;
#endif
    return ob;
}

std::vector<OpenOrderInfo> BinanceExchange::getOpenOrders(const std::string& symbol) {
#ifndef USE_CURL
    (void)symbol;
    return {};
#else
    try {
        std::string path;
        if (marketType_ == "futures") {
            path = "/fapi/v1/openOrders";
        } else {
            path = "/api/v3/openOrders";
        }
        if (!symbol.empty()) path += "?symbol=" + symbol;
        auto resp = httpGet(path, true);
        auto j = nlohmann::json::parse(resp);
        std::vector<OpenOrderInfo> result;
        for (auto& o : j) {
            OpenOrderInfo info;
            info.orderId    = std::to_string(o.value("orderId", (int64_t)0));
            info.symbol     = o.value("symbol", "");
            info.side       = o.value("side", "");
            info.type       = o.value("type", "");
            info.price      = safeStod(o.value("price", "0"));
            info.qty        = safeStod(o.value("origQty", "0"));
            info.executedQty = safeStod(o.value("executedQty", "0"));
            info.time       = o.value("time", (int64_t)0);
            result.push_back(std::move(info));
        }
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Binance] getOpenOrders failed: {}", e.what());
        return {};
    }
#endif
}

std::vector<UserTradeInfo> BinanceExchange::getMyTrades(const std::string& symbol, int limit) {
#ifndef USE_CURL
    (void)symbol; (void)limit;
    return {};
#else
    try {
        std::string path;
        if (marketType_ == "futures") {
            path = "/fapi/v1/userTrades?symbol=" + symbol +
                   "&limit=" + std::to_string(limit);
        } else {
            path = "/api/v3/myTrades?symbol=" + symbol +
                   "&limit=" + std::to_string(limit);
        }
        auto resp = httpGet(path, true);
        auto j = nlohmann::json::parse(resp);
        std::vector<UserTradeInfo> result;
        for (auto& t : j) {
            UserTradeInfo info;
            info.time           = t.value("time", (int64_t)0);
            info.symbol         = t.value("symbol", "");
            info.side           = t.value("isBuyer", false) ? "BUY" : "SELL";
            info.price          = safeStod(t.value("price", "0"));
            info.qty            = safeStod(t.value("qty", "0"));
            info.commission     = safeStod(t.value("commission", "0"));
            info.commissionAsset = t.value("commissionAsset", "");
            result.push_back(std::move(info));
        }
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Binance] getMyTrades failed: {}", e.what());
        return {};
    }
#endif
}

std::vector<PositionInfo> BinanceExchange::getPositionRisk(const std::string& symbol) {
#ifndef USE_CURL
    (void)symbol;
    return {};
#else
    try {
        std::string url = futuresBaseUrl_ + "/fapi/v2/positionRisk";
        if (!symbol.empty()) url += "?symbol=" + symbol;
        auto resp = httpGetUrl(url, true);
        auto j = nlohmann::json::parse(resp);
        if (!j.is_array()) {
            Logger::get()->warn("[Binance] getPositionRisk: unexpected response format");
            return {};
        }
        std::vector<PositionInfo> result;
        for (auto& p : j) {
            PositionInfo info;
            info.symbol           = p.value("symbol", "");
            info.positionAmt      = safeStod(p.value("positionAmt", "0"));
            info.entryPrice       = safeStod(p.value("entryPrice", "0"));
            info.unrealizedProfit = safeStod(p.value("unRealizedProfit", "0")); // Binance API uses camelCase "unRealizedProfit"
            info.marginType       = p.value("marginType", "");
            info.leverage         = safeStod(p.value("leverage", "1"));
            result.push_back(std::move(info));
        }
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Binance] getPositionRisk failed: {}", e.what());
        return {};
    }
#endif
}

FuturesBalanceInfo BinanceExchange::getFuturesBalance() {
#ifndef USE_CURL
    return {};
#else
    try {
        std::string url = futuresBaseUrl_ + "/fapi/v2/account";
        auto resp = httpGetUrl(url, true);
        auto j = nlohmann::json::parse(resp);

        // Guard against error responses that don't contain the expected fields
        if (!j.is_object() || !j.contains("totalWalletBalance")) {
            std::string code = j.value("code", "");
            std::string msg  = j.value("msg", "unexpected response format");
            Logger::get()->warn("[Binance] getFuturesBalance unexpected response: {} {}", code, msg);
            return {};
        }

        FuturesBalanceInfo info;
        info.totalWalletBalance    = safeStod(j.value("totalWalletBalance", "0"));
        info.totalUnrealizedProfit = safeStod(j.value("totalUnrealizedProfit", "0"));
        info.totalMarginBalance    = safeStod(j.value("totalMarginBalance", "0"));
        if (j.contains("positions") && j["positions"].is_array()) {
            for (auto& p : j["positions"]) {
                PositionInfo pos;
                pos.symbol           = p.value("symbol", "");
                pos.positionAmt      = safeStod(p.value("positionAmt", "0"));
                pos.entryPrice       = safeStod(p.value("entryPrice", "0"));
                pos.unrealizedProfit = safeStod(p.value("unrealizedProfit", "0")); // /fapi/v2/account uses lowercase
                info.positions.push_back(std::move(pos));
            }
        }
        return info;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Binance] getFuturesBalance failed: {}", e.what());
        return {};
    }
#endif
}

bool BinanceExchange::cancelOrder(const std::string& symbol, const std::string& orderId) {
#ifndef USE_CURL
    (void)symbol; (void)orderId;
    return false;
#else
    try {
        std::string path;
        if (marketType_ == "futures")
            path = "/fapi/v1/order";
        else
            path = "/api/v3/order";

        std::ostringstream body;
        body << "symbol=" << symbol << "&orderId=" << orderId;
        auto resp = httpPost(path, body.str());
        auto j = nlohmann::json::parse(resp);
        if (j.contains("code")) {
            Logger::get()->warn("[Binance] cancelOrder error: {}", j.value("msg", ""));
            return false;
        }
        Logger::get()->info("[Binance] cancelOrder success: {} {}", symbol, orderId);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Binance] cancelOrder failed: {}", e.what());
        return false;
    }
#endif
}

bool BinanceExchange::setLeverage(const std::string& symbol, int leverage) {
#ifndef USE_CURL
    (void)symbol; (void)leverage;
    return false;
#else
    try {
        std::ostringstream body;
        body << "symbol=" << symbol << "&leverage=" << leverage;
        auto resp = httpPost("/fapi/v1/leverage", body.str());
        auto j = nlohmann::json::parse(resp);
        if (j.contains("code") && j["code"].get<int>() != 200) {
            Logger::get()->warn("[Binance] setLeverage error: {}", j.value("msg", ""));
            return false;
        }
        Logger::get()->info("[Binance] setLeverage: {} = {}x", symbol, leverage);
        return true;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Binance] setLeverage failed: {}", e.what());
        return false;
    }
#endif
}

int BinanceExchange::getLeverage(const std::string& symbol) {
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

} // namespace crypto
