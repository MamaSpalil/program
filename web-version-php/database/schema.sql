-- CryptoTrader Database Schema for Microsoft SQL Server 2012
-- Encoding: UTF-8

-- Create database (run separately if needed)
-- CREATE DATABASE cryptotrader;
-- GO

USE cryptotrader;
GO

-- Drop tables if they exist (for clean installation)
IF OBJECT_ID('trades', 'U') IS NOT NULL DROP TABLE trades;
IF OBJECT_ID('orders', 'U') IS NOT NULL DROP TABLE orders;
IF OBJECT_ID('positions', 'U') IS NOT NULL DROP TABLE positions;
IF OBJECT_ID('alerts', 'U') IS NOT NULL DROP TABLE alerts;
IF OBJECT_ID('backtest_results', 'U') IS NOT NULL DROP TABLE backtest_results;
IF OBJECT_ID('strategies', 'U') IS NOT NULL DROP TABLE strategies;
IF OBJECT_ID('exchange_credentials', 'U') IS NOT NULL DROP TABLE exchange_credentials;
IF OBJECT_ID('sessions', 'U') IS NOT NULL DROP TABLE sessions;
IF OBJECT_ID('users', 'U') IS NOT NULL DROP TABLE users;
GO

-- Users table
CREATE TABLE users (
    user_id INT IDENTITY(1,1) PRIMARY KEY,
    username NVARCHAR(50) NOT NULL UNIQUE,
    email NVARCHAR(100) NOT NULL UNIQUE,
    password_hash NVARCHAR(255) NOT NULL,
    full_name NVARCHAR(100),
    is_active BIT DEFAULT 1,
    is_admin BIT DEFAULT 0,
    email_verified BIT DEFAULT 0,
    two_factor_enabled BIT DEFAULT 0,
    two_factor_secret NVARCHAR(100),
    created_at DATETIME2 DEFAULT GETDATE(),
    updated_at DATETIME2 DEFAULT GETDATE(),
    last_login_at DATETIME2,
    INDEX idx_username (username),
    INDEX idx_email (email)
);
GO

-- Sessions table
CREATE TABLE sessions (
    session_id NVARCHAR(100) PRIMARY KEY,
    user_id INT NOT NULL,
    ip_address NVARCHAR(45),
    user_agent NVARCHAR(255),
    created_at DATETIME2 DEFAULT GETDATE(),
    expires_at DATETIME2 NOT NULL,
    last_activity DATETIME2 DEFAULT GETDATE(),
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_expires_at (expires_at)
);
GO

-- Exchange credentials table (API keys stored encrypted)
CREATE TABLE exchange_credentials (
    credential_id INT IDENTITY(1,1) PRIMARY KEY,
    user_id INT NOT NULL,
    exchange_name NVARCHAR(50) NOT NULL,
    api_key_encrypted NVARCHAR(500) NOT NULL,
    api_secret_encrypted NVARCHAR(500) NOT NULL,
    passphrase_encrypted NVARCHAR(500),
    is_testnet BIT DEFAULT 1,
    is_active BIT DEFAULT 1,
    label NVARCHAR(100),
    created_at DATETIME2 DEFAULT GETDATE(),
    updated_at DATETIME2 DEFAULT GETDATE(),
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_exchange (user_id, exchange_name)
);
GO

-- Trading strategies table
CREATE TABLE strategies (
    strategy_id INT IDENTITY(1,1) PRIMARY KEY,
    user_id INT NOT NULL,
    name NVARCHAR(100) NOT NULL,
    description NVARCHAR(MAX),
    strategy_type NVARCHAR(50) NOT NULL, -- 'ml_enhanced', 'technical', 'custom'
    parameters NVARCHAR(MAX), -- JSON string
    is_active BIT DEFAULT 1,
    is_public BIT DEFAULT 0,
    created_at DATETIME2 DEFAULT GETDATE(),
    updated_at DATETIME2 DEFAULT GETDATE(),
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_strategies (user_id, is_active)
);
GO

-- Positions table
CREATE TABLE positions (
    position_id INT IDENTITY(1,1) PRIMARY KEY,
    user_id INT NOT NULL,
    exchange_name NVARCHAR(50) NOT NULL,
    symbol NVARCHAR(20) NOT NULL,
    side NVARCHAR(10) NOT NULL, -- 'LONG', 'SHORT'
    entry_price DECIMAL(18, 8) NOT NULL,
    quantity DECIMAL(18, 8) NOT NULL,
    leverage INT DEFAULT 1,
    unrealized_pnl DECIMAL(18, 8) DEFAULT 0,
    realized_pnl DECIMAL(18, 8) DEFAULT 0,
    stop_loss DECIMAL(18, 8),
    take_profit DECIMAL(18, 8),
    trailing_stop_percent DECIMAL(5, 2),
    status NVARCHAR(20) DEFAULT 'OPEN', -- 'OPEN', 'CLOSED'
    opened_at DATETIME2 DEFAULT GETDATE(),
    closed_at DATETIME2,
    strategy_id INT,
    notes NVARCHAR(MAX),
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (strategy_id) REFERENCES strategies(strategy_id) ON DELETE SET NULL,
    INDEX idx_user_positions (user_id, status),
    INDEX idx_symbol (symbol),
    INDEX idx_opened_at (opened_at)
);
GO

-- Orders table
CREATE TABLE orders (
    order_id INT IDENTITY(1,1) PRIMARY KEY,
    user_id INT NOT NULL,
    exchange_name NVARCHAR(50) NOT NULL,
    exchange_order_id NVARCHAR(100),
    symbol NVARCHAR(20) NOT NULL,
    order_type NVARCHAR(20) NOT NULL, -- 'MARKET', 'LIMIT', 'STOP', 'STOP_LIMIT'
    side NVARCHAR(10) NOT NULL, -- 'BUY', 'SELL'
    price DECIMAL(18, 8),
    quantity DECIMAL(18, 8) NOT NULL,
    filled_quantity DECIMAL(18, 8) DEFAULT 0,
    status NVARCHAR(20) DEFAULT 'PENDING', -- 'PENDING', 'OPEN', 'FILLED', 'PARTIALLY_FILLED', 'CANCELLED', 'REJECTED'
    time_in_force NVARCHAR(10) DEFAULT 'GTC', -- 'GTC', 'IOC', 'FOK'
    position_id INT,
    strategy_id INT,
    reduce_only BIT DEFAULT 0,
    post_only BIT DEFAULT 0,
    created_at DATETIME2 DEFAULT GETDATE(),
    updated_at DATETIME2 DEFAULT GETDATE(),
    filled_at DATETIME2,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (position_id) REFERENCES positions(position_id) ON DELETE SET NULL,
    FOREIGN KEY (strategy_id) REFERENCES strategies(strategy_id) ON DELETE SET NULL,
    INDEX idx_user_orders (user_id, status),
    INDEX idx_exchange_order_id (exchange_order_id),
    INDEX idx_created_at (created_at)
);
GO

-- Trades table (executed trades)
CREATE TABLE trades (
    trade_id INT IDENTITY(1,1) PRIMARY KEY,
    user_id INT NOT NULL,
    order_id INT,
    exchange_name NVARCHAR(50) NOT NULL,
    exchange_trade_id NVARCHAR(100),
    symbol NVARCHAR(20) NOT NULL,
    side NVARCHAR(10) NOT NULL, -- 'BUY', 'SELL'
    price DECIMAL(18, 8) NOT NULL,
    quantity DECIMAL(18, 8) NOT NULL,
    fee DECIMAL(18, 8) DEFAULT 0,
    fee_currency NVARCHAR(10),
    realized_pnl DECIMAL(18, 8),
    position_id INT,
    executed_at DATETIME2 DEFAULT GETDATE(),
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (order_id) REFERENCES orders(order_id) ON DELETE SET NULL,
    FOREIGN KEY (position_id) REFERENCES positions(position_id) ON DELETE SET NULL,
    INDEX idx_user_trades (user_id),
    INDEX idx_symbol (symbol),
    INDEX idx_executed_at (executed_at)
);
GO

-- Alerts table
CREATE TABLE alerts (
    alert_id INT IDENTITY(1,1) PRIMARY KEY,
    user_id INT NOT NULL,
    symbol NVARCHAR(20) NOT NULL,
    alert_type NVARCHAR(20) NOT NULL, -- 'PRICE_ABOVE', 'PRICE_BELOW', 'PERCENT_CHANGE', 'INDICATOR'
    condition_value DECIMAL(18, 8) NOT NULL,
    current_value DECIMAL(18, 8),
    message NVARCHAR(500),
    is_active BIT DEFAULT 1,
    is_triggered BIT DEFAULT 0,
    triggered_at DATETIME2,
    created_at DATETIME2 DEFAULT GETDATE(),
    expires_at DATETIME2,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_alerts (user_id, is_active),
    INDEX idx_symbol (symbol)
);
GO

-- Backtest results table
CREATE TABLE backtest_results (
    backtest_id INT IDENTITY(1,1) PRIMARY KEY,
    user_id INT NOT NULL,
    strategy_id INT,
    symbol NVARCHAR(20) NOT NULL,
    interval NVARCHAR(10) NOT NULL, -- '1m', '5m', '15m', '1h', '4h', '1d'
    start_date DATETIME2 NOT NULL,
    end_date DATETIME2 NOT NULL,
    initial_capital DECIMAL(18, 8) NOT NULL,
    final_capital DECIMAL(18, 8) NOT NULL,
    total_return_percent DECIMAL(8, 2),
    total_trades INT DEFAULT 0,
    winning_trades INT DEFAULT 0,
    losing_trades INT DEFAULT 0,
    win_rate DECIMAL(5, 2),
    max_drawdown DECIMAL(8, 2),
    sharpe_ratio DECIMAL(8, 4),
    profit_factor DECIMAL(8, 4),
    parameters NVARCHAR(MAX), -- JSON string
    results_data NVARCHAR(MAX), -- JSON string with detailed results
    created_at DATETIME2 DEFAULT GETDATE(),
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (strategy_id) REFERENCES strategies(strategy_id) ON DELETE SET NULL,
    INDEX idx_user_backtest (user_id),
    INDEX idx_created_at (created_at)
);
GO

-- Create stored procedures

-- Procedure to clean up expired sessions
CREATE PROCEDURE sp_cleanup_expired_sessions
AS
BEGIN
    DELETE FROM sessions WHERE expires_at < GETDATE();
END;
GO

-- Procedure to get user portfolio summary
CREATE PROCEDURE sp_get_portfolio_summary
    @user_id INT
AS
BEGIN
    SELECT
        COUNT(DISTINCT p.position_id) as open_positions,
        SUM(p.unrealized_pnl) as total_unrealized_pnl,
        (SELECT COUNT(*) FROM orders WHERE user_id = @user_id AND status IN ('OPEN', 'PENDING')) as open_orders,
        (SELECT COUNT(*) FROM trades WHERE user_id = @user_id AND executed_at >= DATEADD(day, -1, GETDATE())) as trades_today,
        (SELECT SUM(realized_pnl) FROM trades WHERE user_id = @user_id AND executed_at >= DATEADD(day, -1, GETDATE())) as pnl_today,
        (SELECT SUM(realized_pnl) FROM trades WHERE user_id = @user_id AND executed_at >= DATEADD(day, -7, GETDATE())) as pnl_week,
        (SELECT SUM(realized_pnl) FROM trades WHERE user_id = @user_id AND executed_at >= DATEADD(day, -30, GETDATE())) as pnl_month
    FROM positions p
    WHERE p.user_id = @user_id AND p.status = 'OPEN';
END;
GO

-- Procedure to update position PnL
CREATE PROCEDURE sp_update_position_pnl
    @position_id INT,
    @current_price DECIMAL(18, 8)
AS
BEGIN
    UPDATE positions
    SET unrealized_pnl =
        CASE
            WHEN side = 'LONG' THEN ((@current_price - entry_price) * quantity)
            WHEN side = 'SHORT' THEN ((entry_price - @current_price) * quantity)
        END,
        updated_at = GETDATE()
    WHERE position_id = @position_id;
END;
GO

-- Create indexes for performance
CREATE INDEX idx_positions_user_symbol ON positions(user_id, symbol, status);
CREATE INDEX idx_orders_user_symbol ON orders(user_id, symbol, status);
CREATE INDEX idx_trades_user_symbol_date ON trades(user_id, symbol, executed_at);
GO

-- Insert default admin user (password: admin123 - CHANGE IN PRODUCTION!)
-- Password hash for 'admin123' using bcrypt
INSERT INTO users (username, email, password_hash, full_name, is_active, is_admin, email_verified)
VALUES ('admin', 'admin@cryptotrader.local', '$2y$10$92IXUNpkjO0rOQ5byMi.Ye4oKoEa3Ro9llC/.og/at2.uheWG/igi', 'Administrator', 1, 1, 1);
GO

PRINT 'Database schema created successfully!';
PRINT 'Default admin user created: username=admin, password=admin123';
PRINT 'IMPORTANT: Change the admin password in production!';
GO
