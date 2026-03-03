#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

namespace crypto {

class Logger {
public:
    static void init(const std::string& logFile = "logs/trading.log",
                     size_t maxSizeBytes = 10 * 1024 * 1024,
                     size_t maxFiles = 7);
    static std::shared_ptr<spdlog::logger> get();

private:
    static std::shared_ptr<spdlog::logger> logger_;
};

} // namespace crypto
