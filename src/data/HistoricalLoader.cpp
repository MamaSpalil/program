#include "HistoricalLoader.h"
#include "../core/Logger.h"
#ifdef USE_CURL
#include <curl/curl.h>
#endif
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <sstream>

namespace crypto {

#ifdef USE_CURL
namespace {
size_t writeCallback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}
} // namespace
#endif

HistoricalLoader::HistoricalLoader(const std::string& baseUrl,
                                   const std::string& apiKey,
                                   const std::string& apiSecret)
    : baseUrl_(baseUrl), apiKey_(apiKey), apiSecret_(apiSecret) {}

std::string HistoricalLoader::httpGet(const std::string& url) const {
#ifndef USE_CURL
    throw std::runtime_error("libcurl not available: " + url);
#else
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));
    }
    if (httpCode >= 400) {
        throw std::runtime_error("HTTP error " + std::to_string(httpCode) + ": " + response);
    }
    return response;
#endif
}

std::vector<Candle> HistoricalLoader::parseKlines(const std::string& json) const {
    auto arr = nlohmann::json::parse(json);
    std::vector<Candle> candles;
    candles.reserve(arr.size());
    for (auto& k : arr) {
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
        candles.push_back(c);
    }
    return candles;
}

std::vector<Candle> HistoricalLoader::load(const std::string& symbol,
                                            const std::string& interval,
                                            int limit,
                                            int64_t startTime,
                                            int64_t endTime) const {
    std::ostringstream oss;
    oss << baseUrl_ << "/api/v3/klines?symbol=" << symbol
        << "&interval=" << interval << "&limit=" << limit;
    if (startTime > 0) oss << "&startTime=" << startTime;
    if (endTime > 0)   oss << "&endTime=" << endTime;

    auto json = httpGet(oss.str());
    auto candles = parseKlines(json);
    Logger::get()->info("Loaded {} candles for {} {}", candles.size(), symbol, interval);
    return candles;
}

} // namespace crypto
