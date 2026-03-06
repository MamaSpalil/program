#include "FearGreed.h"
#include "../core/Logger.h"
#include <chrono>
#ifdef USE_CURL
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#endif

namespace crypto {

#ifdef USE_CURL
namespace {
size_t fearGreedWriteCallback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}
} // namespace
#endif

FearGreed::~FearGreed() {
    stop();
}

void FearGreed::start() {
    if (running_) return;
    running_ = true;
    pollThread_ = std::thread(&FearGreed::loop, this);
}

void FearGreed::stop() {
    running_ = false;
    if (pollThread_.joinable()) pollThread_.join();
}

FearGreedData FearGreed::getData() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return data_;
}

void FearGreed::setData(const FearGreedData& d) {
    std::lock_guard<std::mutex> lk(mutex_);
    data_ = d;
}

void FearGreed::fetch() {
#ifdef USE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger::get()->warn("[FearGreed] curl_easy_init failed");
        return;
    }

    std::string url = "https://api.alternative.me/fng/?limit=1";
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fearGreedWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CryptoTrader/1.8.0");

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        Logger::get()->warn("[FearGreed] curl error: {}", curl_easy_strerror(res));
        return;
    }
    if (httpCode != 200) {
        Logger::get()->warn("[FearGreed] HTTP {} — skipping parse", httpCode);
        return;
    }

    try {
        auto json = nlohmann::json::parse(response);
        auto dataArr = json.value("data", nlohmann::json::array());
        if (!dataArr.empty()) {
            const auto& entry = dataArr[0];
            FearGreedData d;
            // API returns value as string
            std::string valStr = entry.value("value", "50");
            try { d.value = std::stoi(valStr); } catch (...) { d.value = 50; }
            d.classification = entry.value("value_classification", "Neutral");
            std::string tsStr = entry.value("timestamp", "0");
            try { d.timestamp = std::stoll(tsStr); } catch (...) { d.timestamp = 0; }

            setData(d);
            Logger::get()->debug("[FearGreed] index={} ({}) ts={}",
                                 d.value, d.classification, d.timestamp);
        }
    } catch (const std::exception& e) {
        Logger::get()->warn("[FearGreed] JSON parse error: {}", e.what());
    }
#else
    Logger::get()->debug("[FearGreed] Fetch cycle (libcurl not available)");
#endif
}

void FearGreed::loop() {
    while (running_) {
        fetch();
        // Update every hour
        for (int i = 0; i < 3600 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

} // namespace crypto
