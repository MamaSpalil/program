#include "core/Engine.h"
#include "core/Logger.h"
#include <csignal>
#include <atomic>
#include <iostream>
#include <memory>

namespace {
std::atomic<bool> g_shutdown{false};
crypto::Engine*   g_engine{nullptr};

void signalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_shutdown = true;
        if (g_engine) g_engine->stop();
    }
}
} // namespace

int main(int argc, char* argv[]) {
    std::string configPath = "config/settings.json";
    if (argc > 1) configPath = argv[1];

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        crypto::Logger::init();
        crypto::Logger::get()->info("CryptoTrader starting...");

        auto engine = std::make_unique<crypto::Engine>(configPath);
        g_engine = engine.get();
        engine->run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
