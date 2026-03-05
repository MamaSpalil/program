#include "DatabaseMigrator.h"
#include "../core/Logger.h"
#include <sqlite3.h>

namespace crypto {

DatabaseMigrator::DatabaseMigrator(TradingDatabase& db,
                                   TradeRepository& tradeRepo,
                                   AlertRepository& alertRepo,
                                   DrawingRepository& drawingRepo)
    : db_(db), tradeRepo_(tradeRepo), alertRepo_(alertRepo), drawingRepo_(drawingRepo) {}

void DatabaseMigrator::runIfNeeded() {
    if (isMigrated()) return;

    Logger::get()->info("[DB] Running migration...");
    tradeRepo_.migrateFromJSON("config/trade_history.json");
    alertRepo_.migrateFromJSON("config/alerts.json");
    markMigrated();
    Logger::get()->info("[DB] Migration complete");
}

bool DatabaseMigrator::isMigrated() {
    std::lock_guard<std::mutex> lock(db_.getMutex());

    // Create meta table if not exists
    sqlite3_exec(db_.handle(),
        "CREATE TABLE IF NOT EXISTS _meta (key TEXT PRIMARY KEY, value TEXT)",
        nullptr, nullptr, nullptr);

    const char* sql = "SELECT value FROM _meta WHERE key='migrated'";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    bool migrated = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto* p = sqlite3_column_text(stmt, 0);
        if (p) migrated = (std::string(reinterpret_cast<const char*>(p)) == "1");
    }
    sqlite3_finalize(stmt);
    return migrated;
}

void DatabaseMigrator::markMigrated() {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    sqlite3_exec(db_.handle(),
        "INSERT OR REPLACE INTO _meta (key, value) VALUES ('migrated', '1')",
        nullptr, nullptr, nullptr);
}

} // namespace crypto
