#include "EquityRepository.h"
#include "../core/Logger.h"
#include <sqlite3.h>
#include <chrono>

namespace crypto {

EquityRepository::EquityRepository(TradingDatabase& db) : db_(db) {}

void EquityRepository::push(const EquitySnapshot& snap) {
    std::lock_guard<std::mutex> lock(bufferMutex_);
    buffer_.push_back(snap);
}

void EquityRepository::flush() {
    std::vector<EquitySnapshot> toFlush;
    {
        std::lock_guard<std::mutex> lock(bufferMutex_);
        toFlush.swap(buffer_);
    }
    if (toFlush.empty()) return;

    std::lock_guard<std::mutex> lock(db_.getMutex());
    sqlite3_exec(db_.handle(), "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    const char* sql = R"(
        INSERT INTO equity_history (timestamp, equity, balance, unrealized_pnl, is_paper)
        VALUES (?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db_.handle(), "ROLLBACK", nullptr, nullptr, nullptr);
        return;
    }

    for (const auto& s : toFlush) {
        sqlite3_bind_int64(stmt, 1, s.timestamp);
        sqlite3_bind_double(stmt, 2, s.equity);
        sqlite3_bind_double(stmt, 3, s.balance);
        sqlite3_bind_double(stmt, 4, s.unrealizedPnl);
        sqlite3_bind_int(stmt, 5, s.isPaper ? 1 : 0);
        int stepRc = sqlite3_step(stmt);
        if (stepRc != SQLITE_DONE) {
            Logger::get()->error("EquityRepository: step failed: {}",
                                 sqlite3_errmsg(db_.handle()));
        }
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db_.handle(), "COMMIT", nullptr, nullptr, nullptr);
}

std::vector<EquitySnapshot> EquityRepository::getHistory(bool isPaper, int limit) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<EquitySnapshot> result;
    const char* sql = R"(
        SELECT timestamp, equity, balance, unrealized_pnl, is_paper
        FROM equity_history WHERE is_paper=?
        ORDER BY timestamp DESC LIMIT ?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_int(stmt, 1, isPaper ? 1 : 0);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EquitySnapshot s;
        s.timestamp     = sqlite3_column_int64(stmt, 0);
        s.equity        = sqlite3_column_double(stmt, 1);
        s.balance       = sqlite3_column_double(stmt, 2);
        s.unrealizedPnl = sqlite3_column_double(stmt, 3);
        s.isPaper       = sqlite3_column_int(stmt, 4) != 0;
        result.push_back(s);
    }
    sqlite3_finalize(stmt);
    return result;
}

void EquityRepository::pruneOld(int keepDays) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    long long cutoff = now - static_cast<long long>(keepDays) * 86400000LL;

    const char* sql = "DELETE FROM equity_history WHERE timestamp < ?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_int64(stmt, 1, cutoff);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

} // namespace crypto
