#include "TradeRepository.h"
#include "../core/Logger.h"
#include <sqlite3.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <chrono>

namespace crypto {

namespace {
std::string safeText(sqlite3_stmt* stmt, int col) {
    auto* p = sqlite3_column_text(stmt, col);
    return p ? std::string(reinterpret_cast<const char*>(p)) : std::string();
}
} // namespace

TradeRepository::TradeRepository(TradingDatabase& db) : db_(db) {}

bool TradeRepository::upsert(const TradeRecord& t) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        INSERT OR REPLACE INTO trades
        (id, symbol, exchange, side, type, qty, entry_price, exit_price,
         pnl, commission, entry_time, exit_time, is_paper, strategy,
         status, created_at, updated_at)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, t.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, t.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, t.exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, t.side.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, t.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 6, t.qty);
    sqlite3_bind_double(stmt, 7, t.entryPrice);
    sqlite3_bind_double(stmt, 8, t.exitPrice);
    sqlite3_bind_double(stmt, 9, t.pnl);
    sqlite3_bind_double(stmt, 10, t.commission);
    sqlite3_bind_int64(stmt, 11, t.entryTime);
    sqlite3_bind_int64(stmt, 12, t.exitTime);
    sqlite3_bind_int(stmt, 13, t.isPaper ? 1 : 0);
    sqlite3_bind_text(stmt, 14, t.strategy.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 15, t.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 16, t.createdAt);
    sqlite3_bind_int64(stmt, 17, t.updatedAt);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool TradeRepository::close(const std::string& id, double exitPrice, double pnl, long long exitTime) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        UPDATE trades SET exit_price=?, pnl=?, exit_time=?, status='closed',
        updated_at=? WHERE id=?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_double(stmt, 1, exitPrice);
    sqlite3_bind_double(stmt, 2, pnl);
    sqlite3_bind_int64(stmt, 3, exitTime);
    sqlite3_bind_int64(stmt, 4, exitTime);
    sqlite3_bind_text(stmt, 5, id.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<TradeRecord> TradeRepository::getOpen(bool isPaper) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<TradeRecord> result;
    const char* sql = R"(
        SELECT id, symbol, exchange, side, type, qty, entry_price, exit_price,
               pnl, commission, entry_time, exit_time, is_paper, strategy,
               status, created_at, updated_at
        FROM trades WHERE status='open' AND is_paper=?
        ORDER BY entry_time DESC
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_int(stmt, 1, isPaper ? 1 : 0);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TradeRecord t;
        t.id         = safeText(stmt, 0);
        t.symbol     = safeText(stmt, 1);
        t.exchange   = safeText(stmt, 2);
        t.side       = safeText(stmt, 3);
        t.type       = safeText(stmt, 4);
        t.qty        = sqlite3_column_double(stmt, 5);
        t.entryPrice = sqlite3_column_double(stmt, 6);
        t.exitPrice  = sqlite3_column_double(stmt, 7);
        t.pnl        = sqlite3_column_double(stmt, 8);
        t.commission = sqlite3_column_double(stmt, 9);
        t.entryTime  = sqlite3_column_int64(stmt, 10);
        t.exitTime   = sqlite3_column_int64(stmt, 11);
        t.isPaper    = sqlite3_column_int(stmt, 12) != 0;
        t.strategy   = safeText(stmt, 13);
        t.status     = safeText(stmt, 14);
        t.createdAt  = sqlite3_column_int64(stmt, 15);
        t.updatedAt  = sqlite3_column_int64(stmt, 16);
        result.push_back(std::move(t));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<TradeRecord> TradeRepository::getHistory(const std::string& symbol, int limit) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<TradeRecord> result;
    std::string sql;
    if (symbol.empty()) {
        sql = "SELECT id, symbol, exchange, side, type, qty, entry_price, exit_price, "
              "pnl, commission, entry_time, exit_time, is_paper, strategy, status, "
              "created_at, updated_at FROM trades ORDER BY entry_time DESC LIMIT ?";
    } else {
        sql = "SELECT id, symbol, exchange, side, type, qty, entry_price, exit_price, "
              "pnl, commission, entry_time, exit_time, is_paper, strategy, status, "
              "created_at, updated_at FROM trades WHERE symbol=? ORDER BY entry_time DESC LIMIT ?";
    }
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    if (symbol.empty()) {
        sqlite3_bind_int(stmt, 1, limit);
    } else {
        sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, limit);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TradeRecord t;
        t.id         = safeText(stmt, 0);
        t.symbol     = safeText(stmt, 1);
        t.exchange   = safeText(stmt, 2);
        t.side       = safeText(stmt, 3);
        t.type       = safeText(stmt, 4);
        t.qty        = sqlite3_column_double(stmt, 5);
        t.entryPrice = sqlite3_column_double(stmt, 6);
        t.exitPrice  = sqlite3_column_double(stmt, 7);
        t.pnl        = sqlite3_column_double(stmt, 8);
        t.commission = sqlite3_column_double(stmt, 9);
        t.entryTime  = sqlite3_column_int64(stmt, 10);
        t.exitTime   = sqlite3_column_int64(stmt, 11);
        t.isPaper    = sqlite3_column_int(stmt, 12) != 0;
        t.strategy   = safeText(stmt, 13);
        t.status     = safeText(stmt, 14);
        t.createdAt  = sqlite3_column_int64(stmt, 15);
        t.updatedAt  = sqlite3_column_int64(stmt, 16);
        result.push_back(std::move(t));
    }
    sqlite3_finalize(stmt);
    return result;
}

bool TradeRepository::migrateFromJSON(const std::string& jsonPath) {
    std::ifstream f(jsonPath);
    if (!f.is_open()) {
        Logger::get()->info("[TradeRepo] No JSON file to migrate: {}", jsonPath);
        return true;
    }
    try {
        nlohmann::json j;
        f >> j;
        if (!j.is_array()) return false;

        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        sqlite3_exec(db_.handle(), "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

        for (const auto& item : j) {
            TradeRecord t;
            t.id         = item.value("id", "");
            t.symbol     = item.value("symbol", "");
            t.exchange   = item.value("exchange", "");
            t.side       = item.value("side", "");
            t.type       = item.value("type", "MARKET");
            t.qty        = item.value("qty", 0.0);
            t.entryPrice = item.value("entryPrice", 0.0);
            t.exitPrice  = item.value("exitPrice", 0.0);
            t.pnl        = item.value("pnl", 0.0);
            t.commission = item.value("commission", 0.0);
            t.entryTime  = item.value("entryTime", 0LL);
            t.exitTime   = item.value("exitTime", 0LL);
            t.isPaper    = item.value("isPaper", false);
            t.strategy   = item.value("strategy", "");
            t.status     = (t.exitTime > 0) ? "closed" : "open";
            t.createdAt  = t.entryTime > 0 ? t.entryTime : now;
            t.updatedAt  = now;
            if (!t.id.empty())
                upsert(t);
        }
        sqlite3_exec(db_.handle(), "COMMIT", nullptr, nullptr, nullptr);
        Logger::get()->info("[TradeRepo] Migrated {} trades from {}", j.size(), jsonPath);
    } catch (const std::exception& e) {
        Logger::get()->warn("[TradeRepo] Migration error: {}", e.what());
        return false;
    }
    return true;
}

} // namespace crypto
