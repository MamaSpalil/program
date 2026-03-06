#include "TradingDatabase.h"
#include "../core/Logger.h"
#include <sqlite3.h>

namespace crypto {

TradingDatabase::TradingDatabase(const std::string& path)
    : path_(path) {}

TradingDatabase::~TradingDatabase() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool TradingDatabase::init() {
    int rc = sqlite3_open(path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err = db_ ? sqlite3_errmsg(db_) : "unknown error";
        Logger::get()->error("[TradingDB] Cannot open {}: {}", path_, err);
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }
    applyPragmas();
    return createSchema();
}

bool TradingDatabase::isOpen() const {
    return db_ != nullptr;
}

bool TradingDatabase::execute(const std::string& sql) {
    if (!db_) {
        Logger::get()->warn("[TradingDB] execute() called with null db handle");
        return false;
    }
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "unknown";
        sqlite3_free(errMsg);
        Logger::get()->warn("[TradingDB] SQL error: {}", err);
        return false;
    }
    return true;
}

void TradingDatabase::applyPragmas() {
    execute("PRAGMA journal_mode=WAL");
    execute("PRAGMA synchronous=NORMAL");
    execute("PRAGMA foreign_keys=ON");
    execute("PRAGMA cache_size=-32000");
    execute("PRAGMA temp_store=MEMORY");
    execute("PRAGMA mmap_size=268435456");
}

bool TradingDatabase::createSchema() {
    execute("BEGIN TRANSACTION");

    // Table 1: trades
    execute(R"(
        CREATE TABLE IF NOT EXISTS trades (
            id            TEXT PRIMARY KEY,
            symbol        TEXT NOT NULL,
            exchange      TEXT NOT NULL,
            side          TEXT NOT NULL,
            type          TEXT NOT NULL,
            qty           REAL NOT NULL,
            entry_price   REAL NOT NULL,
            exit_price    REAL NOT NULL DEFAULT 0,
            pnl           REAL NOT NULL DEFAULT 0,
            commission    REAL NOT NULL DEFAULT 0,
            entry_time    INTEGER NOT NULL,
            exit_time     INTEGER NOT NULL DEFAULT 0,
            is_paper      INTEGER NOT NULL DEFAULT 0,
            strategy      TEXT NOT NULL DEFAULT '',
            status        TEXT NOT NULL DEFAULT 'open',
            created_at    INTEGER NOT NULL,
            updated_at    INTEGER NOT NULL
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_trades_symbol
        ON trades(symbol, exchange, entry_time DESC)
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_trades_status
        ON trades(status, is_paper)
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_trades_time
        ON trades(entry_time DESC)
    )");

    // Table 2: orders
    execute(R"(
        CREATE TABLE IF NOT EXISTS orders (
            id                TEXT PRIMARY KEY,
            exchange_order_id TEXT NOT NULL DEFAULT '',
            symbol            TEXT NOT NULL,
            exchange          TEXT NOT NULL,
            side              TEXT NOT NULL,
            type              TEXT NOT NULL,
            qty               REAL NOT NULL,
            price             REAL NOT NULL DEFAULT 0,
            stop_price        REAL NOT NULL DEFAULT 0,
            filled_qty        REAL NOT NULL DEFAULT 0,
            avg_fill_price    REAL NOT NULL DEFAULT 0,
            status            TEXT NOT NULL DEFAULT 'NEW',
            is_paper          INTEGER NOT NULL DEFAULT 0,
            reduce_only       INTEGER NOT NULL DEFAULT 0,
            created_at        INTEGER NOT NULL,
            updated_at        INTEGER NOT NULL
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_orders_symbol
        ON orders(symbol, exchange, status)
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_orders_status
        ON orders(status, created_at DESC)
    )");

    // Table 3: positions
    execute(R"(
        CREATE TABLE IF NOT EXISTS positions (
            id              TEXT PRIMARY KEY,
            symbol          TEXT NOT NULL,
            exchange        TEXT NOT NULL,
            side            TEXT NOT NULL,
            qty             REAL NOT NULL,
            entry_price     REAL NOT NULL,
            current_price   REAL NOT NULL DEFAULT 0,
            unrealized_pnl  REAL NOT NULL DEFAULT 0,
            realized_pnl    REAL NOT NULL DEFAULT 0,
            tp_price        REAL NOT NULL DEFAULT 0,
            sl_price        REAL NOT NULL DEFAULT 0,
            leverage        REAL NOT NULL DEFAULT 1,
            is_paper        INTEGER NOT NULL DEFAULT 0,
            opened_at       INTEGER NOT NULL,
            updated_at      INTEGER NOT NULL,
            UNIQUE(symbol, exchange, side, is_paper)
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_positions_symbol
        ON positions(symbol, exchange, is_paper)
    )");

    // Table 4: equity_history
    execute(R"(
        CREATE TABLE IF NOT EXISTS equity_history (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp      INTEGER NOT NULL,
            equity         REAL NOT NULL,
            balance        REAL NOT NULL,
            unrealized_pnl REAL NOT NULL DEFAULT 0,
            is_paper       INTEGER NOT NULL DEFAULT 0
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_equity_time
        ON equity_history(timestamp DESC, is_paper)
    )");

    // Table 5: alerts
    execute(R"(
        CREATE TABLE IF NOT EXISTS alerts (
            id              TEXT PRIMARY KEY,
            symbol          TEXT NOT NULL,
            condition       TEXT NOT NULL,
            threshold       REAL NOT NULL,
            enabled         INTEGER NOT NULL DEFAULT 1,
            triggered       INTEGER NOT NULL DEFAULT 0,
            trigger_count   INTEGER NOT NULL DEFAULT 0,
            last_triggered  INTEGER NOT NULL DEFAULT 0,
            notify_type     TEXT NOT NULL DEFAULT 'sound',
            created_at      INTEGER NOT NULL,
            updated_at      INTEGER NOT NULL
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_alerts_symbol
        ON alerts(symbol, enabled)
    )");

    // Table 6: backtest_results
    execute(R"(
        CREATE TABLE IF NOT EXISTS backtest_results (
            id              TEXT PRIMARY KEY,
            symbol          TEXT NOT NULL,
            timeframe       TEXT NOT NULL,
            date_from       INTEGER NOT NULL,
            date_to         INTEGER NOT NULL,
            initial_balance REAL NOT NULL,
            final_balance   REAL NOT NULL,
            pnl             REAL NOT NULL,
            pnl_pct         REAL NOT NULL DEFAULT 0,
            win_rate        REAL NOT NULL DEFAULT 0,
            sharpe          REAL NOT NULL DEFAULT 0,
            max_drawdown    REAL NOT NULL DEFAULT 0,
            profit_factor   REAL NOT NULL DEFAULT 0,
            total_trades    INTEGER NOT NULL DEFAULT 0,
            winning_trades  INTEGER NOT NULL DEFAULT 0,
            losing_trades   INTEGER NOT NULL DEFAULT 0,
            strategy        TEXT NOT NULL DEFAULT '',
            commission      REAL NOT NULL DEFAULT 0,
            run_at          INTEGER NOT NULL
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_backtest_symbol
        ON backtest_results(symbol, timeframe, run_at DESC)
    )");

    // Table 7: backtest_trades
    execute(R"(
        CREATE TABLE IF NOT EXISTS backtest_trades (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            backtest_id   TEXT NOT NULL
                          REFERENCES backtest_results(id)
                          ON DELETE CASCADE,
            symbol        TEXT NOT NULL,
            side          TEXT NOT NULL,
            entry_price   REAL NOT NULL,
            exit_price    REAL NOT NULL,
            qty           REAL NOT NULL,
            pnl           REAL NOT NULL,
            commission    REAL NOT NULL DEFAULT 0,
            entry_time    INTEGER NOT NULL,
            exit_time     INTEGER NOT NULL
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_bt_trades_id
        ON backtest_trades(backtest_id)
    )");

    // Table 8: drawings
    execute(R"(
        CREATE TABLE IF NOT EXISTS drawings (
            id          TEXT PRIMARY KEY,
            symbol      TEXT NOT NULL,
            exchange    TEXT NOT NULL,
            type        TEXT NOT NULL,
            x1          REAL NOT NULL,
            y1          REAL NOT NULL,
            x2          REAL NOT NULL DEFAULT 0,
            y2          REAL NOT NULL DEFAULT 0,
            color_r     REAL NOT NULL DEFAULT 1,
            color_g     REAL NOT NULL DEFAULT 1,
            color_b     REAL NOT NULL DEFAULT 0,
            color_a     REAL NOT NULL DEFAULT 1,
            thickness   REAL NOT NULL DEFAULT 1.5,
            label       TEXT NOT NULL DEFAULT '',
            created_at  INTEGER NOT NULL
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_drawings_symbol
        ON drawings(symbol, exchange)
    )");

    // Table 9: funding_rates
    execute(R"(
        CREATE TABLE IF NOT EXISTS funding_rates (
            symbol            TEXT NOT NULL,
            exchange          TEXT NOT NULL,
            rate              REAL NOT NULL,
            next_funding_time INTEGER NOT NULL DEFAULT 0,
            recorded_at       INTEGER NOT NULL,
            PRIMARY KEY (symbol, exchange, recorded_at)
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_funding_symbol
        ON funding_rates(symbol, exchange, recorded_at DESC)
    )");

    // Table 10: scanner_cache
    execute(R"(
        CREATE TABLE IF NOT EXISTS scanner_cache (
            symbol      TEXT NOT NULL,
            exchange    TEXT NOT NULL,
            price       REAL NOT NULL DEFAULT 0,
            change_24h  REAL NOT NULL DEFAULT 0,
            volume_24h  REAL NOT NULL DEFAULT 0,
            rsi         REAL NOT NULL DEFAULT 0,
            signal      TEXT NOT NULL DEFAULT 'HOLD',
            confidence  REAL NOT NULL DEFAULT 0,
            updated_at  INTEGER NOT NULL,
            PRIMARY KEY (symbol, exchange)
        )
    )");

    // Table 11: webhook_log
    execute(R"(
        CREATE TABLE IF NOT EXISTS webhook_log (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            received_at INTEGER NOT NULL,
            action      TEXT NOT NULL,
            symbol      TEXT NOT NULL DEFAULT '',
            qty         REAL NOT NULL DEFAULT 0,
            price       REAL NOT NULL DEFAULT 0,
            status      TEXT NOT NULL DEFAULT 'ok',
            error_msg   TEXT NOT NULL DEFAULT '',
            client_ip   TEXT NOT NULL DEFAULT ''
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_webhook_time
        ON webhook_log(received_at DESC)
    )");

    // Table 12: tax_events
    execute(R"(
        CREATE TABLE IF NOT EXISTS tax_events (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            tax_year     INTEGER NOT NULL,
            method       TEXT NOT NULL,
            symbol       TEXT NOT NULL,
            qty          REAL NOT NULL,
            proceeds     REAL NOT NULL,
            cost_basis   REAL NOT NULL,
            gain_loss    REAL NOT NULL,
            open_time    INTEGER NOT NULL,
            close_time   INTEGER NOT NULL,
            is_long_term INTEGER NOT NULL DEFAULT 0,
            calculated_at INTEGER NOT NULL
        )
    )");
    execute(R"(
        CREATE INDEX IF NOT EXISTS idx_tax_year
        ON tax_events(tax_year, symbol)
    )");

    execute("COMMIT");
    return true;
}

bool TradingDatabase::tableExists(const std::string& tableName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, tableName.c_str(), -1, SQLITE_TRANSIENT);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

} // namespace crypto
