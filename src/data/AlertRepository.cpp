#include "AlertRepository.h"
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

AlertRepository::AlertRepository(TradingDatabase& db) : db_(db) {}

bool AlertRepository::insert(const AlertRecord& a) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        INSERT INTO alerts
        (id, symbol, condition, threshold, enabled, triggered, trigger_count,
         last_triggered, notify_type, created_at, updated_at)
        VALUES (?,?,?,?,?,?,?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, a.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, a.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, a.condition.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, a.threshold);
    sqlite3_bind_int(stmt, 5, a.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 6, a.triggered ? 1 : 0);
    sqlite3_bind_int(stmt, 7, a.triggerCount);
    sqlite3_bind_int64(stmt, 8, a.lastTriggered);
    sqlite3_bind_text(stmt, 9, a.notifyType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 10, a.createdAt);
    sqlite3_bind_int64(stmt, 11, a.updatedAt);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool AlertRepository::update(const AlertRecord& a) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        UPDATE alerts SET symbol=?, condition=?, threshold=?, enabled=?,
        triggered=?, trigger_count=?, last_triggered=?, notify_type=?, updated_at=?
        WHERE id=?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, a.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, a.condition.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, a.threshold);
    sqlite3_bind_int(stmt, 4, a.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 5, a.triggered ? 1 : 0);
    sqlite3_bind_int(stmt, 6, a.triggerCount);
    sqlite3_bind_int64(stmt, 7, a.lastTriggered);
    sqlite3_bind_text(stmt, 8, a.notifyType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 9, a.updatedAt);
    sqlite3_bind_text(stmt, 10, a.id.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool AlertRepository::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = "DELETE FROM alerts WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool AlertRepository::setTriggered(const std::string& id, long long time) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = R"(
        UPDATE alerts SET triggered=1, last_triggered=?,
        trigger_count=trigger_count+1, updated_at=? WHERE id=?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, time);
    sqlite3_bind_int64(stmt, 2, time);
    sqlite3_bind_text(stmt, 3, id.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool AlertRepository::resetTriggered(const std::string& id) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const char* sql = "UPDATE alerts SET triggered=0, updated_at=? WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<AlertRecord> AlertRepository::getAll() const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<AlertRecord> result;
    const char* sql = R"(
        SELECT id, symbol, condition, threshold, enabled, triggered,
               trigger_count, last_triggered, notify_type, created_at, updated_at
        FROM alerts ORDER BY created_at DESC
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AlertRecord a;
        a.id            = safeText(stmt, 0);
        a.symbol        = safeText(stmt, 1);
        a.condition     = safeText(stmt, 2);
        a.threshold     = sqlite3_column_double(stmt, 3);
        a.enabled       = sqlite3_column_int(stmt, 4) != 0;
        a.triggered     = sqlite3_column_int(stmt, 5) != 0;
        a.triggerCount  = sqlite3_column_int(stmt, 6);
        a.lastTriggered = sqlite3_column_int64(stmt, 7);
        a.notifyType    = safeText(stmt, 8);
        a.createdAt     = sqlite3_column_int64(stmt, 9);
        a.updatedAt     = sqlite3_column_int64(stmt, 10);
        result.push_back(std::move(a));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<AlertRecord> AlertRepository::getEnabled() const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<AlertRecord> result;
    const char* sql = R"(
        SELECT id, symbol, condition, threshold, enabled, triggered,
               trigger_count, last_triggered, notify_type, created_at, updated_at
        FROM alerts WHERE enabled=1 ORDER BY created_at DESC
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AlertRecord a;
        a.id            = safeText(stmt, 0);
        a.symbol        = safeText(stmt, 1);
        a.condition     = safeText(stmt, 2);
        a.threshold     = sqlite3_column_double(stmt, 3);
        a.enabled       = sqlite3_column_int(stmt, 4) != 0;
        a.triggered     = sqlite3_column_int(stmt, 5) != 0;
        a.triggerCount  = sqlite3_column_int(stmt, 6);
        a.lastTriggered = sqlite3_column_int64(stmt, 7);
        a.notifyType    = safeText(stmt, 8);
        a.createdAt     = sqlite3_column_int64(stmt, 9);
        a.updatedAt     = sqlite3_column_int64(stmt, 10);
        result.push_back(std::move(a));
    }
    sqlite3_finalize(stmt);
    return result;
}

bool AlertRepository::migrateFromJSON(const std::string& jsonPath) {
    std::ifstream f(jsonPath);
    if (!f.is_open()) {
        Logger::get()->info("[AlertRepo] No JSON file to migrate: {}", jsonPath);
        return true;
    }
    try {
        nlohmann::json j;
        f >> j;
        if (!j.is_array()) return false;

        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        for (const auto& item : j) {
            AlertRecord a;
            a.id         = item.value("id", "");
            a.symbol     = item.value("symbol", "");
            a.condition  = item.value("condition", "");
            a.threshold  = item.value("threshold", 0.0);
            a.enabled    = item.value("enabled", true);
            a.triggered  = item.value("triggered", false);
            a.notifyType = item.value("notifyType", "sound");
            a.createdAt  = now;
            a.updatedAt  = now;
            if (!a.id.empty())
                insert(a);
        }
        Logger::get()->info("[AlertRepo] Migrated {} alerts from {}", j.size(), jsonPath);
    } catch (const std::exception& e) {
        Logger::get()->warn("[AlertRepo] Migration error: {}", e.what());
        return false;
    }
    return true;
}

} // namespace crypto
