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
                                   const std::string& wsPort)
    : apiKey_(apiKey), apiSecret_(apiSecret),
      baseUrl_(baseUrl), wsHost_(wsHost), wsPort_(wsPort) {}

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

void BinanceExchange::rateLimit() const {
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

std::string BinanceExchange::httpGet(const std::string& path, bool signed_) const {
#ifndef USE_CURL
    throw std::runtime_error("libcurl not available");
    (void)path; (void)signed_;
#else
    rateLimit();
    std::string url = baseUrl_ + path;
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
        url = baseUrl_ + path +
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

    std::string url = baseUrl_ + path;
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
    c.open      = std::stod(k[1].get<std::string>());
    c.high      = std::stod(k[2].get<std::string>());
    c.low       = std::stod(k[3].get<std::string>());
    c.close     = std::stod(k[4].get<std::string>());
    c.volume    = std::stod(k[5].get<std::string>());
    c.closeTime = k[6].get<int64_t>();
    c.trades    = k[8].get<int64_t>();
    c.closed    = true;
    return c;
}

std::vector<Candle> BinanceExchange::getKlines(const std::string& symbol,
                                                 const std::string& interval,
                                                 int limit) {
    Logger::get()->debug("[Binance] getKlines symbol={} interval={} limit={}", symbol, interval, limit);
    std::string path = "/api/v3/klines?symbol=" + symbol +
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
    auto json = nlohmann::json::parse(httpGet("/api/v3/ticker/price?symbol=" + symbol));
    double price = std::stod(json["price"].get<std::string>());
    Logger::get()->debug("[Binance] getPrice result={}", price);
    return price;
}

OrderResponse BinanceExchange::placeOrder(const OrderRequest& req) {
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

    auto json = nlohmann::json::parse(httpPost("/api/v3/order", body.str()));
    OrderResponse resp;
    resp.orderId      = json.contains("orderId") ? std::to_string(json["orderId"].get<long>()) : "";
    resp.status       = json.value("status", "UNKNOWN");
    resp.executedQty  = std::stod(json.value("executedQty", "0"));
    resp.executedPrice= std::stod(json.value("price", "0"));
    return resp;
}

AccountBalance BinanceExchange::getBalance() {
    auto json = nlohmann::json::parse(httpGet("/api/v3/account", true));
    AccountBalance bal;
    for (auto& asset : json["balances"]) {
        std::string a = asset["asset"].get<std::string>();
        double free = std::stod(asset["free"].get<std::string>());
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
        c.open      = std::stod(k["o"].get<std::string>());
        c.high      = std::stod(k["h"].get<std::string>());
        c.low       = std::stod(k["l"].get<std::string>());
        c.close     = std::stod(k["c"].get<std::string>());
        c.volume    = std::stod(k["v"].get<std::string>());
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
    klineCb_ = std::move(cb);
    std::string path = "/stream?streams=" +
        [&]{ std::string s = symbol; for(auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; }() +
        "@kline_" + interval;
    ws_ = std::make_unique<WebSocketClient>(wsHost_, wsPort_, path);
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
    try {
        auto resp = httpGet("/api/v3/exchangeInfo");
        auto j = nlohmann::json::parse(resp);
        std::vector<SymbolInfo> result;
        if (j.contains("symbols") && j["symbols"].is_array()) {
            for (auto& s : j["symbols"]) {
                if (s.value("status", "") != "TRADING") continue;
                SymbolInfo si;
                si.symbol     = s.value("symbol", "");
                si.baseAsset  = s.value("baseAsset", "");
                si.quoteAsset = s.value("quoteAsset", "");
                si.marketType = "spot";
                si.displayName = si.baseAsset + "/" + si.quoteAsset;
                if (marketType.empty() || marketType == si.marketType)
                    result.push_back(std::move(si));
            }
        }
        Logger::get()->info("[Binance] Fetched {} symbols", result.size());
        return result;
    } catch (const std::exception& e) {
        Logger::get()->warn("[Binance] getSymbols failed: {}", e.what());
        return {};
    }
#endif
}

OrderBook BinanceExchange::getOrderBook(const std::string& symbol, int depth) {
    OrderBook ob;
#ifdef USE_CURL
    try {
        Logger::get()->debug("[Binance] getOrderBook symbol={} depth={}", symbol, depth);
        std::string path = "/api/v3/depth?symbol=" + symbol +
                           "&limit=" + std::to_string(depth);
        auto response = httpGet(path);
        auto json = nlohmann::json::parse(response);

        ob.lastUpdateId = json.value("lastUpdateId", (int64_t)0);
        for (auto& b : json["bids"]) {
            OrderBook::Level lvl;
            lvl.price = std::stod(b[0].get<std::string>());
            lvl.qty   = std::stod(b[1].get<std::string>());
            ob.bids.push_back(lvl);
        }
        for (auto& a : json["asks"]) {
            OrderBook::Level lvl;
            lvl.price = std::stod(a[0].get<std::string>());
            lvl.qty   = std::stod(a[1].get<std::string>());
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

} // namespace crypto
