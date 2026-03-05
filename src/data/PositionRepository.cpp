#include "PositionRepository.h"
#include "../core/Logger.h"
#include <sqlite3.h>
#include <chrono>

namespace crypto {

namespace {
std::string safeText(sqlite3_stmt* stmt, int col) {
    auto* p = sqlite3_column_text(stmt, col);
    return p ? std::string(reinterpret_cast<const char*>(p)) : std::string();
}
} // namespace

PositionRepository::PositionRepository(TradingDatabase& db) : db_(db) {}

bool PositionRepository::upsert(const PositionRecord& p) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        INSERT OR REPLACE INTO positions
        (id, symbol, exchange, side, qty, entry_price, current_price,
         unrealized_pnl, realized_pnl, tp_price, sl_price, leverage,
         is_paper, opened_at, updated_at)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, p.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, p.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, p.exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, p.side.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, p.qty);
    sqlite3_bind_double(stmt, 6, p.entryPrice);
    sqlite3_bind_double(stmt, 7, p.currentPrice);
    sqlite3_bind_double(stmt, 8, p.unrealizedPnl);
    sqlite3_bind_double(stmt, 9, p.realizedPnl);
    sqlite3_bind_double(stmt, 10, p.tpPrice);
    sqlite3_bind_double(stmt, 11, p.slPrice);
    sqlite3_bind_double(stmt, 12, p.leverage);
    sqlite3_bind_int(stmt, 13, p.isPaper ? 1 : 0);
    sqlite3_bind_int64(stmt, 14, p.openedAt);
    sqlite3_bind_int64(stmt, 15, p.updatedAt);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool PositionRepository::updatePrice(const std::string& symbol, const std::string& exchange,
                                     bool isPaper, double currentPrice, double unrealizedPnl) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const char* sql = R"(
        UPDATE positions SET current_price=?, unrealized_pnl=?, updated_at=?
        WHERE symbol=? AND exchange=? AND is_paper=?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_double(stmt, 1, currentPrice);
    sqlite3_bind_double(stmt, 2, unrealizedPnl);
    sqlite3_bind_int64(stmt, 3, now);
    sqlite3_bind_text(stmt, 4, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, isPaper ? 1 : 0);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool PositionRepository::updateTPSL(const std::string& symbol, const std::string& exchange,
                                    bool isPaper, double tp, double sl) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const char* sql = R"(
        UPDATE positions SET tp_price=?, sl_price=?, updated_at=?
        WHERE symbol=? AND exchange=? AND is_paper=?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_double(stmt, 1, tp);
    sqlite3_bind_double(stmt, 2, sl);
    sqlite3_bind_int64(stmt, 3, now);
    sqlite3_bind_text(stmt, 4, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, isPaper ? 1 : 0);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::optional<PositionRecord> PositionRepository::get(const std::string& symbol,
                                                       const std::string& exchange,
                                                       bool isPaper) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        SELECT id, symbol, exchange, side, qty, entry_price, current_price,
               unrealized_pnl, realized_pnl, tp_price, sl_price, leverage,
               is_paper, opened_at, updated_at
        FROM positions WHERE symbol=? AND exchange=? AND is_paper=?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, isPaper ? 1 : 0);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        PositionRecord p;
        p.id            = safeText(stmt, 0);
        p.symbol        = safeText(stmt, 1);
        p.exchange      = safeText(stmt, 2);
        p.side          = safeText(stmt, 3);
        p.qty           = sqlite3_column_double(stmt, 4);
        p.entryPrice    = sqlite3_column_double(stmt, 5);
        p.currentPrice  = sqlite3_column_double(stmt, 6);
        p.unrealizedPnl = sqlite3_column_double(stmt, 7);
        p.realizedPnl   = sqlite3_column_double(stmt, 8);
        p.tpPrice       = sqlite3_column_double(stmt, 9);
        p.slPrice       = sqlite3_column_double(stmt, 10);
        p.leverage      = sqlite3_column_double(stmt, 11);
        p.isPaper       = sqlite3_column_int(stmt, 12) != 0;
        p.openedAt      = sqlite3_column_int64(stmt, 13);
        p.updatedAt     = sqlite3_column_int64(stmt, 14);
        sqlite3_finalize(stmt);
        return p;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<PositionRecord> PositionRepository::getAll(bool isPaper) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<PositionRecord> result;
    const char* sql = R"(
        SELECT id, symbol, exchange, side, qty, entry_price, current_price,
               unrealized_pnl, realized_pnl, tp_price, sl_price, leverage,
               is_paper, opened_at, updated_at
        FROM positions WHERE is_paper=?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_int(stmt, 1, isPaper ? 1 : 0);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PositionRecord p;
        p.id            = safeText(stmt, 0);
        p.symbol        = safeText(stmt, 1);
        p.exchange      = safeText(stmt, 2);
        p.side          = safeText(stmt, 3);
        p.qty           = sqlite3_column_double(stmt, 4);
        p.entryPrice    = sqlite3_column_double(stmt, 5);
        p.currentPrice  = sqlite3_column_double(stmt, 6);
        p.unrealizedPnl = sqlite3_column_double(stmt, 7);
        p.realizedPnl   = sqlite3_column_double(stmt, 8);
        p.tpPrice       = sqlite3_column_double(stmt, 9);
        p.slPrice       = sqlite3_column_double(stmt, 10);
        p.leverage      = sqlite3_column_double(stmt, 11);
        p.isPaper       = sqlite3_column_int(stmt, 12) != 0;
        p.openedAt      = sqlite3_column_int64(stmt, 13);
        p.updatedAt     = sqlite3_column_int64(stmt, 14);
        result.push_back(std::move(p));
    }
    sqlite3_finalize(stmt);
    return result;
}

bool PositionRepository::remove(const std::string& symbol, const std::string& exchange, bool isPaper) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = "DELETE FROM positions WHERE symbol=? AND exchange=? AND is_paper=?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, isPaper ? 1 : 0);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

} // namespace crypto
