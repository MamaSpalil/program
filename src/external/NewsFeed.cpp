#include "NewsFeed.h"
#include "../core/Logger.h"
#include <algorithm>
#include <chrono>
#ifdef USE_CURL
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#endif

namespace crypto {

#ifdef USE_CURL
namespace {
size_t newsFeedWriteCallback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}
} // namespace
#endif

NewsFeed::~NewsFeed() {
    stop();
}

void NewsFeed::start() {
    if (running_) return;
    running_ = true;
    pollThread_ = std::thread(&NewsFeed::loop, this);
}

void NewsFeed::stop() {
    running_ = false;
    if (pollThread_.joinable()) pollThread_.join();
}

void NewsFeed::addItem(const NewsItem& item) {
    std::lock_guard<std::mutex> lk(mutex_);
    items_.insert(items_.begin(), item);
    // Keep max 200 items
    if (items_.size() > 200) items_.resize(200);
}

std::vector<NewsItem> NewsFeed::getItems(int n) const {
    std::lock_guard<std::mutex> lk(mutex_);
    int count = std::min(n, static_cast<int>(items_.size()));
    return std::vector<NewsItem>(items_.begin(), items_.begin() + count);
}

int NewsFeed::itemCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(items_.size());
}

void NewsFeed::clear() {
    std::lock_guard<std::mutex> lk(mutex_);
    items_.clear();
}

void NewsFeed::fetchAndParse() {
#ifdef USE_CURL
    if (apiKey_.empty()) {
        Logger::get()->debug("[NewsFeed] Fetch cycle (no API key configured)");
        return;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger::get()->warn("[NewsFeed] curl_easy_init failed");
        return;
    }

    std::string url = "https://cryptopanic.com/api/v1/posts/?auth_token="
                      + apiKey_ + "&public=true&kind=news";
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, newsFeedWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CryptoTrader/1.8.0");

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        Logger::get()->warn("[NewsFeed] curl error: {}", curl_easy_strerror(res));
        return;
    }
    if (httpCode != 200) {
        Logger::get()->warn("[NewsFeed] HTTP {} — skipping parse", httpCode);
        return;
    }

    try {
        auto json = nlohmann::json::parse(response);
        auto results = json.value("results", nlohmann::json::array());

        for (const auto& r : results) {
            NewsItem item;
            item.title   = r.value("title", "");
            item.url     = r.value("url", "");
            item.source  = r.contains("source") ? r["source"].value("title", "") : "";
            item.pubTime = 0; // CryptoPanic uses ISO strings; parse if needed
            // Sentiment: CryptoPanic provides votes (positive/negative/important/liked)
            if (r.contains("votes")) {
                int pos = r["votes"].value("positive", 0);
                int neg = r["votes"].value("negative", 0);
                if (pos > neg) item.sentiment = "positive";
                else if (neg > pos) item.sentiment = "negative";
                else item.sentiment = "neutral";
            } else {
                item.sentiment = "neutral";
            }
            addItem(item);
        }

        Logger::get()->debug("[NewsFeed] Fetched {} news items", results.size());
    } catch (const std::exception& e) {
        Logger::get()->warn("[NewsFeed] JSON parse error: {}", e.what());
    }
#else
    Logger::get()->debug("[NewsFeed] Fetch cycle (libcurl not available)");
#endif
}

void NewsFeed::loop() {
    while (running_) {
        fetchAndParse();
        // Sleep 5 minutes between fetches
        for (int i = 0; i < 300 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

} // namespace crypto
