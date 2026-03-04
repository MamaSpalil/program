#include "CandleCache.h"
#include "../core/Logger.h"
#include <sqlite3.h>
#include <stdexcept>

namespace crypto {

CandleCache::CandleCache(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err = db_ ? sqlite3_errmsg(db_) : "unknown error";
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("CandleCache: cannot open DB " + dbPath + ": " + err);
    }
    // Enable WAL mode for better concurrent read performance
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    initSchema();
}

CandleCache::~CandleCache() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void CandleCache::initSchema() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS candles ("
        "  exchange  TEXT NOT NULL,"
        "  symbol    TEXT NOT NULL,"
        "  interval  TEXT NOT NULL,"
        "  openTime  INTEGER NOT NULL,"
        "  closeTime INTEGER NOT NULL,"
        "  open      REAL NOT NULL,"
        "  high      REAL NOT NULL,"
        "  low       REAL NOT NULL,"
        "  close     REAL NOT NULL,"
        "  volume    REAL NOT NULL,"
        "  trades    INTEGER NOT NULL DEFAULT 0,"
        "  PRIMARY KEY (exchange, symbol, interval, openTime)"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "unknown";
        sqlite3_free(errMsg);
        throw std::runtime_error("CandleCache: schema init failed: " + err);
    }
}

void CandleCache::store(const std::string& exchange,
                         const std::string& symbol,
                         const std::string& interval,
                         const std::vector<Candle>& candles) {
    if (candles.empty()) return;
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    const char* sql =
        "INSERT OR REPLACE INTO candles "
        "(exchange, symbol, interval, openTime, closeTime, open, high, low, close, volume, trades) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        Logger::get()->warn("CandleCache::store prepare failed: {}", sqlite3_errmsg(db_));
        return;
    }

    for (const auto& c : candles) {
        sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, interval.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 4, c.openTime);
        sqlite3_bind_int64(stmt, 5, c.closeTime);
        sqlite3_bind_double(stmt, 6, c.open);
        sqlite3_bind_double(stmt, 7, c.high);
        sqlite3_bind_double(stmt, 8, c.low);
        sqlite3_bind_double(stmt, 9, c.close);
        sqlite3_bind_double(stmt, 10, c.volume);
        sqlite3_bind_int64(stmt, 11, c.trades);

        int stepRc = sqlite3_step(stmt);
        if (stepRc != SQLITE_DONE && stepRc != SQLITE_ROW) {
            Logger::get()->warn("CandleCache::store step failed: {}", sqlite3_errmsg(db_));
        }
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
}

std::vector<Candle> CandleCache::load(const std::string& exchange,
                                       const std::string& symbol,
                                       const std::string& interval) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Candle> result;

    const char* sql =
        "SELECT openTime, closeTime, open, high, low, close, volume, trades "
        "FROM candles "
        "WHERE exchange = ? AND symbol = ? AND interval = ? "
        "ORDER BY openTime ASC;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, interval.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Candle c;
        c.openTime  = sqlite3_column_int64(stmt, 0);
        c.closeTime = sqlite3_column_int64(stmt, 1);
        c.open      = sqlite3_column_double(stmt, 2);
        c.high      = sqlite3_column_double(stmt, 3);
        c.low       = sqlite3_column_double(stmt, 4);
        c.close     = sqlite3_column_double(stmt, 5);
        c.volume    = sqlite3_column_double(stmt, 6);
        c.trades    = sqlite3_column_int64(stmt, 7);
        c.closed    = true;
        result.push_back(c);
    }

    sqlite3_finalize(stmt);
    return result;
}

int64_t CandleCache::latestOpenTime(const std::string& exchange,
                                     const std::string& symbol,
                                     const std::string& interval) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql =
        "SELECT MAX(openTime) FROM candles "
        "WHERE exchange = ? AND symbol = ? AND interval = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return 0;

    sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, interval.c_str(), -1, SQLITE_TRANSIENT);

    int64_t result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        result = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return result;
}

int CandleCache::count(const std::string& exchange,
                        const std::string& symbol,
                        const std::string& interval) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql =
        "SELECT COUNT(*) FROM candles "
        "WHERE exchange = ? AND symbol = ? AND interval = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return 0;

    sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, interval.c_str(), -1, SQLITE_TRANSIENT);

    int result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return result;
}

void CandleCache::clear(const std::string& exchange,
                         const std::string& symbol,
                         const std::string& interval) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql =
        "DELETE FROM candles WHERE exchange = ? AND symbol = ? AND interval = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, interval.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void CandleCache::clearAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3_exec(db_, "DELETE FROM candles;", nullptr, nullptr, nullptr);
}

} // namespace crypto
