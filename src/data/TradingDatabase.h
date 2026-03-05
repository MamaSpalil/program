#pragma once
#include <string>
#include <vector>
#include <mutex>

struct sqlite3;

namespace crypto {

/// SQLite3-based persistent storage for all trading data.
/// Manages config/trading.db with 12 tables for trades, orders, positions, etc.
/// Thread-safe: all access through repositories uses a shared mutex.
class TradingDatabase {
public:
    explicit TradingDatabase(const std::string& path = "config/trading.db");
    ~TradingDatabase();

    TradingDatabase(const TradingDatabase&) = delete;
    TradingDatabase& operator=(const TradingDatabase&) = delete;

    /// Initialize DB: apply pragmas and create all tables
    bool init();
    bool isOpen() const;

    /// Direct handle for repositories
    sqlite3* handle() { return db_; }

    /// Shared mutex for all repositories
    std::mutex& getMutex() { return mutex_; }

    /// Check if a table exists (used in tests)
    bool tableExists(const std::string& tableName) const;

    /// Execute raw SQL (used by maintenance)
    bool execute(const std::string& sql);

private:
    sqlite3*    db_   = nullptr;
    std::string path_;
    mutable std::mutex mutex_;

    void applyPragmas();
    bool createSchema();
};

} // namespace crypto
