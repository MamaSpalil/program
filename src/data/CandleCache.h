#pragma once
#include "CandleData.h"
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

struct sqlite3;

namespace crypto {

/// SQLite3-based local cache for historical candle (bar) data.
/// Stores candles keyed by (exchange, symbol, interval, openTime).
/// Thread-safe: all public methods are protected by a mutex.
class CandleCache {
public:
    /// Open (or create) the SQLite database at the given path.
    explicit CandleCache(const std::string& dbPath = "config/candles.db");
    ~CandleCache();

    CandleCache(const CandleCache&) = delete;
    CandleCache& operator=(const CandleCache&) = delete;

    /// Store a batch of candles for a given exchange/symbol/interval.
    /// Existing candles with the same openTime are replaced (upsert).
    void store(const std::string& exchange,
               const std::string& symbol,
               const std::string& interval,
               const std::vector<Candle>& candles);

    /// Load all cached candles for a given exchange/symbol/interval,
    /// ordered by openTime ascending.
    std::vector<Candle> load(const std::string& exchange,
                             const std::string& symbol,
                             const std::string& interval) const;

    /// Return the latest openTime stored for a given key, or 0 if none.
    int64_t latestOpenTime(const std::string& exchange,
                           const std::string& symbol,
                           const std::string& interval) const;

    /// Return the number of cached candles for a given key.
    int count(const std::string& exchange,
              const std::string& symbol,
              const std::string& interval) const;

    /// Remove all candles for a given exchange/symbol/interval.
    void clear(const std::string& exchange,
               const std::string& symbol,
               const std::string& interval);

    /// Remove all data from the cache.
    void clearAll();

private:
    void initSchema();

    sqlite3* db_{nullptr};
    mutable std::mutex mutex_;
};

} // namespace crypto
