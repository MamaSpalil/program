#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <sstream>
#include <ctime>
#include <nlohmann/json.hpp>

namespace crypto {

struct ScheduledJob {
    std::string id;
    std::string name;
    std::string cronExpr;   // "* * * * *" => min hour dom month dow
    std::string symbol;
    std::string strategy;
    bool        enabled   = true;
    std::string lastRun;
    std::string nextRun;
    std::function<void()> callback;
};

class Scheduler {
public:
    Scheduler() = default;
    ~Scheduler();

    void start();
    void stop();

    void addJob(const ScheduledJob& j);
    void removeJob(const std::string& id);
    std::vector<ScheduledJob> getJobs() const;

    void save(const std::string& path = "config/scheduler.json") const;
    void load(const std::string& path = "config/scheduler.json");

    // Exposed for testing
    bool shouldRun(const std::string& expr, const std::tm& now) const;
    std::string calcNextRun(const std::string& expr, const std::tm& now) const;

    bool isRunning() const { return running_; }

private:
    void loop();
    static std::string formatTime(std::time_t t);

    // Parse a single cron field against a value.
    // Supports: "*", single number, comma-separated, and ranges (e.g. "1-5").
    static bool matchField(const std::string& field, int value);

    std::vector<ScheduledJob> jobs_;
    std::thread     thread_;
    mutable std::mutex mutex_;
    std::atomic<bool> running_{false};
    int nextId_{1};
};

} // namespace crypto
