#include "AuxRepository.h"
#include "../core/Logger.h"
#include <sqlite3.h>
#include <chrono>

namespace crypto {

namespace {
std::string safeText(sqlite3_stmt* stmt, int col) {
    auto* p = sqlite3_column_text(stmt, col);
    return p ? std::string(reinterpret_cast<const char*>(p)) : std::string();
}

long long nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
} // namespace

AuxRepository::AuxRepository(TradingDatabase& db) : db_(db) {}

// ── Funding Rate ────────────────────────────────────────────────────────────

bool AuxRepository::saveFundingRate(const std::string& symbol, const std::string& exchange,
                                    double rate, long long nextFundingTime) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        INSERT OR REPLACE INTO funding_rates
        (symbol, exchange, rate, next_funding_time, recorded_at)
        VALUES (?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, rate);
    sqlite3_bind_int64(stmt, 4, nextFundingTime);
    sqlite3_bind_int64(stmt, 5, nowMs());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<FundingRateRecord> AuxRepository::getFundingHistory(
    const std::string& symbol, const std::string& exchange, int limit) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<FundingRateRecord> result;
    const char* sql = R"(
        SELECT symbol, exchange, rate, next_funding_time, recorded_at
        FROM funding_rates WHERE symbol=? AND exchange=?
        ORDER BY recorded_at DESC LIMIT ?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FundingRateRecord r;
        r.symbol          = safeText(stmt, 0);
        r.exchange        = safeText(stmt, 1);
        r.rate            = sqlite3_column_double(stmt, 2);
        r.nextFundingTime = sqlite3_column_int64(stmt, 3);
        r.recordedAt      = sqlite3_column_int64(stmt, 4);
        result.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return result;
}

void AuxRepository::pruneFundingRates(int keepDays) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    long long cutoff = nowMs() - static_cast<long long>(keepDays) * 86400000LL;
    const char* sql = "DELETE FROM funding_rates WHERE recorded_at < ?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_int64(stmt, 1, cutoff);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ── Scanner Cache ───────────────────────────────────────────────────────────

bool AuxRepository::upsertScanResult(const ScanResult& r, const std::string& exchange) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        INSERT OR REPLACE INTO scanner_cache
        (symbol, exchange, price, change_24h, volume_24h, rsi, signal, confidence, updated_at)
        VALUES (?,?,?,?,?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, r.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, r.price);
    sqlite3_bind_double(stmt, 4, r.change24h);
    sqlite3_bind_double(stmt, 5, r.volume24h);
    sqlite3_bind_double(stmt, 6, r.rsi);
    sqlite3_bind_text(stmt, 7, r.signal.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 8, r.confidence);
    sqlite3_bind_int64(stmt, 9, nowMs());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<ScanResult> AuxRepository::getScanCache(const std::string& exchange) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<ScanResult> result;
    const char* sql = R"(
        SELECT symbol, price, change_24h, volume_24h, rsi, signal, confidence
        FROM scanner_cache WHERE exchange=?
        ORDER BY volume_24h DESC
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_text(stmt, 1, exchange.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ScanResult r;
        r.symbol     = safeText(stmt, 0);
        r.price      = sqlite3_column_double(stmt, 1);
        r.change24h  = sqlite3_column_double(stmt, 2);
        r.volume24h  = sqlite3_column_double(stmt, 3);
        r.rsi        = sqlite3_column_double(stmt, 4);
        r.signal     = safeText(stmt, 5);
        r.confidence = sqlite3_column_double(stmt, 6);
        result.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return result;
}

void AuxRepository::clearStaleScanCache(int olderThanSec) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    long long cutoff = nowMs() - static_cast<long long>(olderThanSec) * 1000LL;
    const char* sql = "DELETE FROM scanner_cache WHERE updated_at < ?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_int64(stmt, 1, cutoff);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ── Webhook Log ─────────────────────────────────────────────────────────────

bool AuxRepository::logWebhook(const WebhookLogEntry& entry) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        INSERT INTO webhook_log
        (received_at, action, symbol, qty, price, status, error_msg, client_ip)
        VALUES (?,?,?,?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, entry.receivedAt);
    sqlite3_bind_text(stmt, 2, entry.action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, entry.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, entry.qty);
    sqlite3_bind_double(stmt, 5, entry.price);
    sqlite3_bind_text(stmt, 6, entry.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, entry.errorMsg.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, entry.clientIp.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<WebhookLogEntry> AuxRepository::getWebhookLog(int limit) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<WebhookLogEntry> result;
    const char* sql = R"(
        SELECT received_at, action, symbol, qty, price, status, error_msg, client_ip
        FROM webhook_log ORDER BY received_at DESC LIMIT ?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        WebhookLogEntry e;
        e.receivedAt = sqlite3_column_int64(stmt, 0);
        e.action     = safeText(stmt, 1);
        e.symbol     = safeText(stmt, 2);
        e.qty        = sqlite3_column_double(stmt, 3);
        e.price      = sqlite3_column_double(stmt, 4);
        e.status     = safeText(stmt, 5);
        e.errorMsg   = safeText(stmt, 6);
        e.clientIp   = safeText(stmt, 7);
        result.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return result;
}

void AuxRepository::pruneWebhookLog(int keepCount) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        DELETE FROM webhook_log WHERE id NOT IN (
            SELECT id FROM webhook_log ORDER BY received_at DESC LIMIT ?
        )
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_int(stmt, 1, keepCount);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ── Tax Events ──────────────────────────────────────────────────────────────

bool AuxRepository::saveTaxEvents(const std::vector<TaxReporter::TaxEvent>& events,
                                  int taxYear, const std::string& method) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    long long now = nowMs();
    sqlite3_exec(db_.handle(), "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    const char* sql = R"(
        INSERT INTO tax_events
        (tax_year, method, symbol, qty, proceeds, cost_basis, gain_loss,
         open_time, close_time, is_long_term, calculated_at)
        VALUES (?,?,?,?,?,?,?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db_.handle(), "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }

    for (const auto& e : events) {
        sqlite3_bind_int(stmt, 1, taxYear);
        sqlite3_bind_text(stmt, 2, method.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, e.symbol.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, e.qty);
        sqlite3_bind_double(stmt, 5, e.proceeds);
        sqlite3_bind_double(stmt, 6, e.costBasis);
        sqlite3_bind_double(stmt, 7, e.gainLoss);
        sqlite3_bind_int64(stmt, 8, e.openTime);
        sqlite3_bind_int64(stmt, 9, e.closeTime);
        sqlite3_bind_int(stmt, 10, e.isLongTerm ? 1 : 0);
        sqlite3_bind_int64(stmt, 11, now);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db_.handle(), "COMMIT", nullptr, nullptr, nullptr);
    return true;
}

std::vector<TaxReporter::TaxEvent> AuxRepository::getTaxEvents(int taxYear,
                                                                 const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<TaxReporter::TaxEvent> result;
    std::string sql;
    if (symbol.empty()) {
        sql = R"(SELECT symbol, qty, proceeds, cost_basis, gain_loss,
                  open_time, close_time, is_long_term
                  FROM tax_events WHERE tax_year=? ORDER BY close_time ASC)";
    } else {
        sql = R"(SELECT symbol, qty, proceeds, cost_basis, gain_loss,
                  open_time, close_time, is_long_term
                  FROM tax_events WHERE tax_year=? AND symbol=? ORDER BY close_time ASC)";
    }
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_int(stmt, 1, taxYear);
    if (!symbol.empty())
        sqlite3_bind_text(stmt, 2, symbol.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TaxReporter::TaxEvent e;
        e.symbol     = safeText(stmt, 0);
        e.qty        = sqlite3_column_double(stmt, 1);
        e.proceeds   = sqlite3_column_double(stmt, 2);
        e.costBasis  = sqlite3_column_double(stmt, 3);
        e.gainLoss   = sqlite3_column_double(stmt, 4);
        e.openTime   = sqlite3_column_int64(stmt, 5);
        e.closeTime  = sqlite3_column_int64(stmt, 6);
        e.isLongTerm = sqlite3_column_int(stmt, 7) != 0;
        result.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return result;
}

void AuxRepository::clearTaxEvents(int taxYear) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = "DELETE FROM tax_events WHERE tax_year=?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_int(stmt, 1, taxYear);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

} // namespace crypto
