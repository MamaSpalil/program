#include "DrawingRepository.h"
#include "../core/Logger.h"
#include <sqlite3.h>
#include <chrono>

namespace crypto {

namespace {
std::string safeText(sqlite3_stmt* stmt, int col) {
    auto* p = sqlite3_column_text(stmt, col);
    return p ? std::string(reinterpret_cast<const char*>(p)) : std::string();
}

std::string drawingTypeToStr(DrawingType t) {
    switch (t) {
        case DrawingType::TREND_LINE:      return "TREND_LINE";
        case DrawingType::HORIZONTAL_LINE: return "HORIZONTAL_LINE";
        case DrawingType::FIBONACCI:       return "FIBONACCI";
        case DrawingType::RECTANGLE:       return "RECTANGLE";
        case DrawingType::TEXT_LABEL:      return "TEXT_LABEL";
        default: return "TREND_LINE";
    }
}

DrawingType strToDrawingType(const std::string& s) {
    if (s == "HORIZONTAL_LINE") return DrawingType::HORIZONTAL_LINE;
    if (s == "FIBONACCI")       return DrawingType::FIBONACCI;
    if (s == "RECTANGLE")       return DrawingType::RECTANGLE;
    if (s == "TEXT_LABEL")      return DrawingType::TEXT_LABEL;
    return DrawingType::TREND_LINE;
}
} // namespace

DrawingRepository::DrawingRepository(TradingDatabase& db) : db_(db) {}

bool DrawingRepository::insert(const DrawingObject& d, const std::string& exchange) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Extract RGBA from uint32_t color
    float r = ((d.color >> 0)  & 0xFF) / 255.0f;
    float g = ((d.color >> 8)  & 0xFF) / 255.0f;
    float b = ((d.color >> 16) & 0xFF) / 255.0f;
    float a = ((d.color >> 24) & 0xFF) / 255.0f;

    const char* sql = R"(
        INSERT OR REPLACE INTO drawings
        (id, symbol, exchange, type, x1, y1, x2, y2, color_r, color_g, color_b,
         color_a, thickness, label, created_at)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, d.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, "", -1, SQLITE_TRANSIENT);  // symbol not in DrawingObject
    sqlite3_bind_text(stmt, 3, exchange.c_str(), -1, SQLITE_TRANSIENT);
    std::string typeStr = drawingTypeToStr(d.type);
    sqlite3_bind_text(stmt, 4, typeStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, d.x1);
    sqlite3_bind_double(stmt, 6, d.y1);
    sqlite3_bind_double(stmt, 7, d.x2);
    sqlite3_bind_double(stmt, 8, d.y2);
    sqlite3_bind_double(stmt, 9, r);
    sqlite3_bind_double(stmt, 10, g);
    sqlite3_bind_double(stmt, 11, b);
    sqlite3_bind_double(stmt, 12, a);
    sqlite3_bind_double(stmt, 13, d.thickness);
    sqlite3_bind_text(stmt, 14, d.text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 15, now);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool DrawingRepository::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = "DELETE FROM drawings WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool DrawingRepository::update(const DrawingObject& d) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    float r = ((d.color >> 0)  & 0xFF) / 255.0f;
    float g = ((d.color >> 8)  & 0xFF) / 255.0f;
    float b = ((d.color >> 16) & 0xFF) / 255.0f;
    float a = ((d.color >> 24) & 0xFF) / 255.0f;

    const char* sql = R"(
        UPDATE drawings SET type=?, x1=?, y1=?, x2=?, y2=?,
        color_r=?, color_g=?, color_b=?, color_a=?, thickness=?, label=?
        WHERE id=?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    std::string typeStr = drawingTypeToStr(d.type);
    sqlite3_bind_text(stmt, 1, typeStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, d.x1);
    sqlite3_bind_double(stmt, 3, d.y1);
    sqlite3_bind_double(stmt, 4, d.x2);
    sqlite3_bind_double(stmt, 5, d.y2);
    sqlite3_bind_double(stmt, 6, r);
    sqlite3_bind_double(stmt, 7, g);
    sqlite3_bind_double(stmt, 8, b);
    sqlite3_bind_double(stmt, 9, a);
    sqlite3_bind_double(stmt, 10, d.thickness);
    sqlite3_bind_text(stmt, 11, d.text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, d.id.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

void DrawingRepository::clear(const std::string& symbol, const std::string& exchange) {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    const char* sql = "DELETE FROM drawings WHERE symbol=? AND exchange=?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<DrawingObject> DrawingRepository::load(const std::string& symbol,
                                                    const std::string& exchange) const {
    std::lock_guard<std::mutex> lock(db_.getMutex());
    std::vector<DrawingObject> result;
    const char* sql = R"(
        SELECT id, type, x1, y1, x2, y2, color_r, color_g, color_b, color_a,
               thickness, label
        FROM drawings WHERE symbol=? AND exchange=?
    )";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, exchange.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DrawingObject d;
        d.id        = safeText(stmt, 0);
        d.type      = strToDrawingType(safeText(stmt, 1));
        d.x1        = sqlite3_column_double(stmt, 2);
        d.y1        = sqlite3_column_double(stmt, 3);
        d.x2        = sqlite3_column_double(stmt, 4);
        d.y2        = sqlite3_column_double(stmt, 5);
        // Reconstruct uint32_t color from RGBA floats
        uint8_t cr = static_cast<uint8_t>(sqlite3_column_double(stmt, 6) * 255.0);
        uint8_t cg = static_cast<uint8_t>(sqlite3_column_double(stmt, 7) * 255.0);
        uint8_t cb = static_cast<uint8_t>(sqlite3_column_double(stmt, 8) * 255.0);
        uint8_t ca = static_cast<uint8_t>(sqlite3_column_double(stmt, 9) * 255.0);
        d.color     = (static_cast<uint32_t>(ca) << 24) |
                      (static_cast<uint32_t>(cb) << 16) |
                      (static_cast<uint32_t>(cg) << 8)  |
                      static_cast<uint32_t>(cr);
        d.thickness = static_cast<float>(sqlite3_column_double(stmt, 10));
        d.text      = safeText(stmt, 11);
        result.push_back(std::move(d));
    }
    sqlite3_finalize(stmt);
    return result;
}

} // namespace crypto
