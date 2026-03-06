#include "Scheduler.h"
#include "../core/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <chrono>

namespace crypto {

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread(&Scheduler::loop, this);
}

void Scheduler::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void Scheduler::addJob(const ScheduledJob& j) {
    std::lock_guard<std::mutex> lk(mutex_);
    ScheduledJob job = j;
    if (job.id.empty()) {
        job.id = "job_" + std::to_string(nextId_++);
    }
    jobs_.push_back(job);
}

void Scheduler::removeJob(const std::string& id) {
    std::lock_guard<std::mutex> lk(mutex_);
    jobs_.erase(
        std::remove_if(jobs_.begin(), jobs_.end(),
            [&](const ScheduledJob& j) { return j.id == id; }),
        jobs_.end());
}

std::vector<ScheduledJob> Scheduler::getJobs() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return jobs_;
}

bool Scheduler::matchField(const std::string& field, int value) {
    if (field == "*") return true;

    // Split by comma
    std::stringstream ss(field);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Check for range (e.g., "1-5")
        auto dash = token.find('-');
        if (dash != std::string::npos) {
            int lo = std::stoi(token.substr(0, dash));
            int hi = std::stoi(token.substr(dash + 1));
            if (value >= lo && value <= hi) return true;
        } else {
            if (std::stoi(token) == value) return true;
        }
    }
    return false;
}

bool Scheduler::shouldRun(const std::string& expr, const std::tm& now) const {
    // Parse cron expression: min hour dom month dow
    std::istringstream iss(expr);
    std::string minF, hourF, domF, monF, dowF;
    if (!(iss >> minF >> hourF >> domF >> monF >> dowF)) return false;

    return matchField(minF, now.tm_min)
        && matchField(hourF, now.tm_hour)
        && matchField(domF, now.tm_mday)
        && matchField(monF, now.tm_mon + 1)  // tm_mon is 0-based
        && matchField(dowF, now.tm_wday);     // tm_wday: 0=Sunday
}

std::string Scheduler::formatTime(std::time_t t) {
    std::tm tm_val;
#ifdef _WIN32
    localtime_s(&tm_val, &t);
#else
    localtime_r(&t, &tm_val);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Scheduler::calcNextRun(const std::string& expr, const std::tm& now) const {
    // Simple: advance minute by minute (up to 7 days) to find next match
    std::tm candidate = now;
    candidate.tm_sec = 0;
    for (int i = 0; i < 10080; ++i) { // 7 days * 24h * 60m
        candidate.tm_min += 1;
        std::mktime(&candidate); // normalize
        if (shouldRun(expr, candidate)) {
            std::time_t t = std::mktime(&candidate);
            return formatTime(t);
        }
    }
    return "N/A";
}

void Scheduler::loop() {
    while (running_) {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now;
#ifdef _WIN32
        localtime_s(&tm_now, &t);
#else
        localtime_r(&t, &tm_now);
#endif
        // Collect callbacks to run outside the mutex to avoid deadlocks
        std::vector<std::pair<std::string, std::function<void()>>> pendingCallbacks;
        {
            std::lock_guard<std::mutex> lk(mutex_);
            for (auto& job : jobs_) {
                if (!job.enabled) continue;
                if (shouldRun(job.cronExpr, tm_now)) {
                    job.lastRun = formatTime(t);
                    job.nextRun = calcNextRun(job.cronExpr, tm_now);
                    Logger::get()->info("[Scheduler] Triggered job: {} ({})",
                                        job.name, job.symbol);
                    if (job.callback) {
                        pendingCallbacks.emplace_back(job.name, job.callback);
                    }
                }
            }
        }
        // Execute callbacks outside the lock
        for (auto& [name, cb] : pendingCallbacks) {
            try {
                cb();
            } catch (const std::exception& e) {
                Logger::get()->warn("[Scheduler] Job {} failed: {}",
                                    name, e.what());
            }
        }
        // Sleep 30 seconds between checks
        for (int i = 0; i < 30 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void Scheduler::save(const std::string& path) const {
    std::lock_guard<std::mutex> lk(mutex_);
    nlohmann::json arr = nlohmann::json::array();
    for (auto& j : jobs_) {
        arr.push_back({
            {"id", j.id}, {"name", j.name}, {"cron", j.cronExpr},
            {"symbol", j.symbol}, {"strategy", j.strategy},
            {"enabled", j.enabled}, {"lastRun", j.lastRun},
            {"nextRun", j.nextRun}
        });
    }
    std::ofstream f(path);
    if (f.is_open()) f << arr.dump(2);
}

void Scheduler::load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;
    try {
        auto arr = nlohmann::json::parse(f);
        std::lock_guard<std::mutex> lk(mutex_);
        jobs_.clear();
        for (auto& j : arr) {
            ScheduledJob job;
            job.id       = j.value("id", "");
            job.name     = j.value("name", "");
            job.cronExpr = j.value("cron", "");
            job.symbol   = j.value("symbol", "");
            job.strategy = j.value("strategy", "");
            job.enabled  = j.value("enabled", true);
            job.lastRun  = j.value("lastRun", "");
            job.nextRun  = j.value("nextRun", "");
            jobs_.push_back(job);
        }
    } catch (...) {
        Logger::get()->warn("[Scheduler] Failed to parse {}", path);
    }
}

} // namespace crypto
