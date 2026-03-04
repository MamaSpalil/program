#include "NewsFeed.h"
#include "../core/Logger.h"
#include <algorithm>
#include <chrono>

namespace crypto {

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
    // Placeholder: In production, this would fetch from CryptoPanic API
    // using libcurl and parse JSON response
    Logger::get()->debug("[NewsFeed] Fetch cycle (no API key configured)");
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
