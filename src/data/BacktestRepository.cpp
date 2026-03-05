#include "BacktestRepository.h"
#include "../core/Logger.h"
#include <sqlite3.h>

namespace crypto {

namespace {
std::string safeText(sqlite3_stmt* stmt, int col) {
    auto* p = sqlite3_column_text(stmt, col);
    return p ? std::string(reinterpret_cast<const char*>(p)) : std::string();
}
} // namespace

BacktestRepository::BacktestRepository(TradingDatabase& db) : db_(db) {}

bool BacktestRepository::save(const BacktestResult& result,
                              const std::vector<BtTradeRecord>& trades) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    sqlite3_exec(db_.handle(), "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    // Insert result
    const char* sqlResult = R"(
        INSERT OR REPLACE INTO backtest_results
        (id, symbol, timeframe, date_from, date_to, initial_balance, final_balance,
         pnl, pnl_pct, win_rate, sharpe, max_drawdown, profit_factor,
         total_trades, winning_trades, losing_trades, strategy, commission, run_at)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sqlResult, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db_.handle(), "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }

    sqlite3_bind_text(stmt, 1, result.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, result.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, result.timeframe.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, result.dateFrom);
    sqlite3_bind_int64(stmt, 5, result.dateTo);
    sqlite3_bind_double(stmt, 6, result.initialBalance);
    sqlite3_bind_double(stmt, 7, result.finalBalance);
    sqlite3_bind_double(stmt, 8, result.pnl);
    sqlite3_bind_double(stmt, 9, result.pnlPct);
    sqlite3_bind_double(stmt, 10, result.winRate);
    sqlite3_bind_double(stmt, 11, result.sharpe);
    sqlite3_bind_double(stmt, 12, result.maxDrawdown);
    sqlite3_bind_double(stmt, 13, result.profitFactor);
    sqlite3_bind_int(stmt, 14, result.totalTrades);
    sqlite3_bind_int(stmt, 15, result.winningTrades);
    sqlite3_bind_int(stmt, 16, result.losingTrades);
    sqlite3_bind_text(stmt, 17, result.strategy.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 18, result.commission);
    sqlite3_bind_int64(stmt, 19, result.runAt);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db_.handle(), "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_finalize(stmt);

    // Insert trades
    const char* sqlTrade = R"(
        INSERT INTO backtest_trades
        (backtest_id, symbol, side, entry_price, exit_price, qty, pnl,
         commission, entry_time, exit_time)
        VALUES (?,?,?,?,?,?,?,?,?,?)
    )";
    rc = sqlite3_prepare_v2(db_.handle(), sqlTrade, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db_.handle(), "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }

    for (const auto& t : trades) {
        sqlite3_bind_text(stmt, 1, t.backtestId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, t.symbol.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, t.side.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, t.entryPrice);
        sqlite3_bind_double(stmt, 5, t.exitPrice);
        sqlite3_bind_double(stmt, 6, t.qty);
        sqlite3_bind_double(stmt, 7, t.pnl);
        sqlite3_bind_double(stmt, 8, t.commission);
        sqlite3_bind_int64(stmt, 9, t.entryTime);
        sqlite3_bind_int64(stmt, 10, t.exitTime);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);

    sqlite3_exec(db_.handle(), "COMMIT", nullptr, nullptr, nullptr);
    return true;
}

std::vector<BacktestResult> BacktestRepository::getAll(const std::string& symbol, int limit) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<BacktestResult> result;
    std::string sql;
    if (symbol.empty()) {
        sql = R"(SELECT id, symbol, timeframe, date_from, date_to, initial_balance,
                  final_balance, pnl, pnl_pct, win_rate, sharpe, max_drawdown,
                  profit_factor, total_trades, winning_trades, losing_trades,
                  strategy, commission, run_at
                  FROM backtest_results ORDER BY run_at DESC LIMIT ?)";
    } else {
        sql = R"(SELECT id, symbol, timeframe, date_from, date_to, initial_balance,
                  final_balance, pnl, pnl_pct, win_rate, sharpe, max_drawdown,
                  profit_factor, total_trades, winning_trades, losing_trades,
                  strategy, commission, run_at
                  FROM backtest_results WHERE symbol=? ORDER BY run_at DESC LIMIT ?)";
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
        BacktestResult r;
        r.id             = safeText(stmt, 0);
        r.symbol         = safeText(stmt, 1);
        r.timeframe      = safeText(stmt, 2);
        r.dateFrom       = sqlite3_column_int64(stmt, 3);
        r.dateTo         = sqlite3_column_int64(stmt, 4);
        r.initialBalance = sqlite3_column_double(stmt, 5);
        r.finalBalance   = sqlite3_column_double(stmt, 6);
        r.pnl            = sqlite3_column_double(stmt, 7);
        r.pnlPct         = sqlite3_column_double(stmt, 8);
        r.winRate        = sqlite3_column_double(stmt, 9);
        r.sharpe         = sqlite3_column_double(stmt, 10);
        r.maxDrawdown    = sqlite3_column_double(stmt, 11);
        r.profitFactor   = sqlite3_column_double(stmt, 12);
        r.totalTrades    = sqlite3_column_int(stmt, 13);
        r.winningTrades  = sqlite3_column_int(stmt, 14);
        r.losingTrades   = sqlite3_column_int(stmt, 15);
        r.strategy       = safeText(stmt, 16);
        r.commission     = sqlite3_column_double(stmt, 17);
        r.runAt          = sqlite3_column_int64(stmt, 18);
        result.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<BtTradeRecord> BacktestRepository::getTrades(const std::string& backtestId) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<BtTradeRecord> result;
    const char* sql = R"(
        SELECT id, backtest_id, symbol, side, entry_price, exit_price, qty,
               pnl, commission, entry_time, exit_time
        FROM backtest_trades WHERE backtest_id=? ORDER BY entry_time ASC
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_text(stmt, 1, backtestId.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        BtTradeRecord t;
        t.id         = sqlite3_column_int(stmt, 0);
        t.backtestId = safeText(stmt, 1);
        t.symbol     = safeText(stmt, 2);
        t.side       = safeText(stmt, 3);
        t.entryPrice = sqlite3_column_double(stmt, 4);
        t.exitPrice  = sqlite3_column_double(stmt, 5);
        t.qty        = sqlite3_column_double(stmt, 6);
        t.pnl        = sqlite3_column_double(stmt, 7);
        t.commission = sqlite3_column_double(stmt, 8);
        t.entryTime  = sqlite3_column_int64(stmt, 9);
        t.exitTime   = sqlite3_column_int64(stmt, 10);
        result.push_back(std::move(t));
    }
    sqlite3_finalize(stmt);
    return result;
}

bool BacktestRepository::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = "DELETE FROM backtest_results WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool BacktestRepository::insertTrade(const BtTradeRecord& t) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        INSERT INTO backtest_trades
        (backtest_id, symbol, side, entry_price, exit_price, qty, pnl,
         commission, entry_time, exit_time)
        VALUES (?,?,?,?,?,?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, t.backtestId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, t.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, t.side.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, t.entryPrice);
    sqlite3_bind_double(stmt, 5, t.exitPrice);
    sqlite3_bind_double(stmt, 6, t.qty);
    sqlite3_bind_double(stmt, 7, t.pnl);
    sqlite3_bind_double(stmt, 8, t.commission);
    sqlite3_bind_int64(stmt, 9, t.entryTime);
    sqlite3_bind_int64(stmt, 10, t.exitTime);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

} // namespace crypto
