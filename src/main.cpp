#include "core/Engine.h"
#include "core/Logger.h"
#include <csignal>
#include <atomic>
#include <iostream>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
std::atomic<bool> g_shutdown{false};
crypto::Engine*   g_engine{nullptr};

void signalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_shutdown = true;
        if (g_engine) g_engine->stop();
    }
}

#ifdef _WIN32
BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_CLOSE_EVENT) {
        g_shutdown = true;
        if (g_engine) g_engine->stop();
        return TRUE;
    }
    return FALSE;
}
#endif
} // namespace

int main(int argc, char* argv[]) {
    std::string configPath = "config/settings.json";
    if (argc > 1) configPath = argv[1];

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

#ifdef _WIN32
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

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
