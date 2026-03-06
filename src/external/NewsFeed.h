#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

namespace crypto {

struct NewsItem {
    std::string title;
    std::string source;
    std::string url;
    long long   pubTime{0};
    std::string sentiment; // "positive" / "negative" / "neutral"
};

class NewsFeed {
public:
    NewsFeed() = default;
    ~NewsFeed();

    void start();
    void stop();
    bool isRunning() const { return running_; }

    void setApiKey(const std::string& key) { apiKey_ = key; }
    void addItem(const NewsItem& item); // For testing / manual injection
    std::vector<NewsItem> getItems(int n = 50) const;
    int itemCount() const;
    void clear();

private:
    void loop();
    void fetchAndParse();

    std::vector<NewsItem>  items_;
    mutable std::mutex     mutex_;
    std::thread            pollThread_;
    std::atomic<bool>      running_{false};
    std::string            apiKey_;
};

} // namespace crypto
