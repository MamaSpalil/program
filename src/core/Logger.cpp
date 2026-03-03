#include "Logger.h"
#include <filesystem>

namespace crypto {

std::shared_ptr<spdlog::logger> Logger::logger_;
std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> Logger::consoleSink_;
bool Logger::debugMode_ = false;

void Logger::init(const std::string& logFile, size_t maxSizeBytes, size_t maxFiles) {
    if (logger_) return;  // already initialized

    std::filesystem::create_directories(
        std::filesystem::path(logFile).parent_path());

    consoleSink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink_->set_level(spdlog::level::info);

    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logFile, maxSizeBytes, maxFiles);
    fileSink->set_level(spdlog::level::debug);

    logger_ = std::make_shared<spdlog::logger>(
        "crypto", spdlog::sinks_init_list{consoleSink_, fileSink});
    logger_->set_level(spdlog::level::debug);
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    spdlog::register_logger(logger_);
    spdlog::set_default_logger(logger_);
}

void Logger::setDebugMode(bool enable) {
    debugMode_ = enable;
    if (consoleSink_) {
        consoleSink_->set_level(enable ? spdlog::level::debug : spdlog::level::info);
    }
    if (logger_) {
        logger_->info("Debug mode {}", enable ? "enabled" : "disabled");
    }
}

bool Logger::isDebugMode() {
    return debugMode_;
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        init();
    }
    return logger_;
}

} // namespace crypto
