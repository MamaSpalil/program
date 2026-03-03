#include "UserIndicatorManager.h"
#include "../core/Logger.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace crypto {

UserIndicatorManager::UserIndicatorManager(const std::string& directory)
    : directory_(directory) {
    // Create the directory if it doesn't exist
    std::error_code ec;
    fs::create_directories(directory_, ec);
}

void UserIndicatorManager::scanAndLoad() {
    std::error_code ec;
    if (!fs::exists(directory_, ec)) return;

    for (const auto& entry : fs::directory_iterator(directory_, ec)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".pine") continue;

        std::string stem = entry.path().stem().string();

        // Skip if already loaded
        if (runtimes_.count(stem)) continue;

        try {
            std::string filepath = entry.path().string();
            Logger::get()->info("Loading Pine Script indicator: {}", filepath);

            PineScript script = PineConverter::parseFile(filepath);
            auto runtime = std::make_unique<PineRuntime>();
            runtime->load(script);

            scripts_[stem] = script;
            runtimes_[stem] = std::move(runtime);

            // Generate C++ code alongside the .pine file
            std::string cppName = "UserIndicator_" + stem;
            std::string cppCode = PineConverter::generateCpp(script, cppName);
            std::string cppPath = entry.path().parent_path().string() + "/" + stem + ".generated.h";
            std::ofstream out(cppPath);
            if (out.is_open()) {
                out << cppCode;
                Logger::get()->info("Generated C++ code: {}", cppPath);
            }

            Logger::get()->info("Loaded indicator '{}' ({} statements)",
                                script.name, script.statements.size());
        } catch (const std::exception& e) {
            Logger::get()->warn("Failed to load {}: {}", stem, e.what());
        }
    }
}

void UserIndicatorManager::updateAll(const Candle& c) {
    for (auto& [name, runtime] : runtimes_) {
        auto plots = runtime->update(c);
        lastPlots_[name] = plots;
    }
}

std::vector<std::string> UserIndicatorManager::loadedIndicators() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : runtimes_) {
        (void)_;
        names.push_back(name);
    }
    return names;
}

std::map<std::string, double>
UserIndicatorManager::getPlots(const std::string& name) const {
    auto it = lastPlots_.find(name);
    return it != lastPlots_.end() ? it->second : std::map<std::string, double>{};
}

std::map<std::string, std::map<std::string, double>>
UserIndicatorManager::getAllPlots() const {
    return lastPlots_;
}

void UserIndicatorManager::generateAllCpp() const {
    for (const auto& [stem, script] : scripts_) {
        std::string cppName = "UserIndicator_" + stem;
        std::string cppCode = PineConverter::generateCpp(script, cppName);
        std::string cppPath = directory_ + "/" + stem + ".generated.h";
        std::ofstream out(cppPath);
        if (out.is_open()) {
            out << cppCode;
            Logger::get()->info("Generated C++ code: {}", cppPath);
        }
    }
}

} // namespace crypto
