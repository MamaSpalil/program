#include "Logger.h"
#include <filesystem>

namespace crypto {

std::shared_ptr<spdlog::logger> Logger::logger_;

void Logger::init(const std::string& logFile, size_t maxSizeBytes, size_t maxFiles) {
    std::filesystem::create_directories(
        std::filesystem::path(logFile).parent_path());

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::info);

    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logFile, maxSizeBytes, maxFiles);
    fileSink->set_level(spdlog::level::debug);

    logger_ = std::make_shared<spdlog::logger>(
        "crypto", spdlog::sinks_init_list{consoleSink, fileSink});
    logger_->set_level(spdlog::level::debug);
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    spdlog::register_logger(logger_);
    spdlog::set_default_logger(logger_);
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        init();
    }
    return logger_;
}

} // namespace crypto
