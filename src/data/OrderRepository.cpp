#include "OrderRepository.h"
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

OrderRepository::OrderRepository(TradingDatabase& db) : db_(db) {}

bool OrderRepository::insert(const OrderRecord& o) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        INSERT OR REPLACE INTO orders
        (id, exchange_order_id, symbol, exchange, side, type, qty, price,
         stop_price, filled_qty, avg_fill_price, status, is_paper,
         reduce_only, created_at, updated_at)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, o.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, o.exchangeOrderId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, o.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, o.exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, o.side.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, o.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 7, o.qty);
    sqlite3_bind_double(stmt, 8, o.price);
    sqlite3_bind_double(stmt, 9, o.stopPrice);
    sqlite3_bind_double(stmt, 10, o.filledQty);
    sqlite3_bind_double(stmt, 11, o.avgFillPrice);
    sqlite3_bind_text(stmt, 12, o.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 13, o.isPaper ? 1 : 0);
    sqlite3_bind_int(stmt, 14, o.reduceOnly ? 1 : 0);
    sqlite3_bind_int64(stmt, 15, o.createdAt);
    sqlite3_bind_int64(stmt, 16, o.updatedAt);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool OrderRepository::updateStatus(const std::string& id, const std::string& status,
                                   double filledQty, double avgFillPrice) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const char* sql = R"(
        UPDATE orders SET status=?, filled_qty=?, avg_fill_price=?, updated_at=?
        WHERE id=?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, filledQty);
    sqlite3_bind_double(stmt, 3, avgFillPrice);
    sqlite3_bind_int64(stmt, 4, now);
    sqlite3_bind_text(stmt, 5, id.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<OrderRecord> OrderRepository::getOpen(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<OrderRecord> result;
    std::string sql;
    if (symbol.empty()) {
        sql = R"(SELECT id, exchange_order_id, symbol, exchange, side, type, qty, price,
                  stop_price, filled_qty, avg_fill_price, status, is_paper,
                  reduce_only, created_at, updated_at
                  FROM orders WHERE status IN ('NEW','PARTIALLY_FILLED')
                  ORDER BY created_at DESC)";
    } else {
        sql = R"(SELECT id, exchange_order_id, symbol, exchange, side, type, qty, price,
                  stop_price, filled_qty, avg_fill_price, status, is_paper,
                  reduce_only, created_at, updated_at
                  FROM orders WHERE status IN ('NEW','PARTIALLY_FILLED') AND symbol=?
                  ORDER BY created_at DESC)";
    }
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    if (!symbol.empty())
        sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OrderRecord o;
        o.id              = safeText(stmt, 0);
        o.exchangeOrderId = safeText(stmt, 1);
        o.symbol          = safeText(stmt, 2);
        o.exchange        = safeText(stmt, 3);
        o.side            = safeText(stmt, 4);
        o.type            = safeText(stmt, 5);
        o.qty             = sqlite3_column_double(stmt, 6);
        o.price           = sqlite3_column_double(stmt, 7);
        o.stopPrice       = sqlite3_column_double(stmt, 8);
        o.filledQty       = sqlite3_column_double(stmt, 9);
        o.avgFillPrice    = sqlite3_column_double(stmt, 10);
        o.status          = safeText(stmt, 11);
        o.isPaper         = sqlite3_column_int(stmt, 12) != 0;
        o.reduceOnly      = sqlite3_column_int(stmt, 13) != 0;
        o.createdAt       = sqlite3_column_int64(stmt, 14);
        o.updatedAt       = sqlite3_column_int64(stmt, 15);
        result.push_back(std::move(o));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<OrderRecord> OrderRepository::getAll(const std::string& symbol, int limit) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<OrderRecord> result;
    std::string sql;
    if (symbol.empty()) {
        sql = R"(SELECT id, exchange_order_id, symbol, exchange, side, type, qty, price,
                  stop_price, filled_qty, avg_fill_price, status, is_paper,
                  reduce_only, created_at, updated_at
                  FROM orders ORDER BY created_at DESC LIMIT ?)";
    } else {
        sql = R"(SELECT id, exchange_order_id, symbol, exchange, side, type, qty, price,
                  stop_price, filled_qty, avg_fill_price, status, is_paper,
                  reduce_only, created_at, updated_at
                  FROM orders WHERE symbol=? ORDER BY created_at DESC LIMIT ?)";
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
        OrderRecord o;
        o.id              = safeText(stmt, 0);
        o.exchangeOrderId = safeText(stmt, 1);
        o.symbol          = safeText(stmt, 2);
        o.exchange        = safeText(stmt, 3);
        o.side            = safeText(stmt, 4);
        o.type            = safeText(stmt, 5);
        o.qty             = sqlite3_column_double(stmt, 6);
        o.price           = sqlite3_column_double(stmt, 7);
        o.stopPrice       = sqlite3_column_double(stmt, 8);
        o.filledQty       = sqlite3_column_double(stmt, 9);
        o.avgFillPrice    = sqlite3_column_double(stmt, 10);
        o.status          = safeText(stmt, 11);
        o.isPaper         = sqlite3_column_int(stmt, 12) != 0;
        o.reduceOnly      = sqlite3_column_int(stmt, 13) != 0;
        o.createdAt       = sqlite3_column_int64(stmt, 14);
        o.updatedAt       = sqlite3_column_int64(stmt, 15);
        result.push_back(std::move(o));
    }
    sqlite3_finalize(stmt);
    return result;
}

bool OrderRepository::deleteOld(int days) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    long long cutoff = now - static_cast<long long>(days) * 86400000LL;

    const char* sql = R"(
        DELETE FROM orders WHERE status IN ('FILLED','CANCELED','REJECTED','EXPIRED')
        AND updated_at < ?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, cutoff);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

} // namespace crypto
