#pragma once
#include "PineConverter.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace crypto {

class UserIndicatorManager {
public:
    explicit UserIndicatorManager(const std::string& directory = "user_indicator");

    /// Scan directory for new/changed .pine files and load them
    void scanAndLoad();

    /// Update all loaded indicators with new candle data
    void updateAll(const Candle& c);

    /// Get names of all loaded indicators
    std::vector<std::string> loadedIndicators() const;

    /// Get plot results for a specific indicator
    std::map<std::string, double> getPlots(const std::string& name) const;

    /// Get all plot results for all indicators
    std::map<std::string, std::map<std::string, double>> getAllPlots() const;

    /// Check if any indicators are loaded
    bool hasIndicators() const { return !runtimes_.empty(); }

    /// Generate C++ source files from all loaded .pine files
    void generateAllCpp() const;

private:
    std::string directory_;
    // filename (stem) -> runtime instance
    std::map<std::string, std::unique_ptr<PineRuntime>> runtimes_;
    // filename (stem) -> parsed script
    std::map<std::string, PineScript> scripts_;
    // Cache of last plot results
    std::map<std::string, std::map<std::string, double>> lastPlots_;
};

} // namespace crypto
