# CryptoTrader Web Version - PHP+Java+HTML+CSS

Web-based cryptocurrency trading platform with ML-enhanced signals using PHP backend, Java ML service, and HTML/CSS frontend with MSSQL 2012 database.

## 🎯 Live Demo

**[Try the Interactive Demo](frontend/demo.html)** - No installation required!

The demo version runs entirely in your browser with simulated data. Features:
- ✅ Interactive TradingView charts with real-time updates
- ✅ Technical indicators (RSI, MACD, EMA, ATR)
- ✅ ML-enhanced trading signals
- ✅ Simulated trading with position tracking
- ✅ Responsive design for all devices
- ✅ No backend/database required

Simply open `frontend/demo.html` in any modern web browser, or view it on [GitHub Pages](https://mamaspalil.github.io/program/web-version-php/frontend/demo.html).

**Want to deploy this demo yourself?** See [GITHUB_PAGES.md](GITHUB_PAGES.md) for instructions.

## Technology Stack

### Backend
- **Language:** PHP 8.1+
- **Database:** Microsoft SQL Server 2012
- **Web Server:** Apache 2.4+ or Nginx with PHP-FPM
- **Extensions:** php-sqlsrv, php-mbstring, php-json, php-curl, php-openssl

### ML Service
- **Language:** Java 17+
- **Framework:** Spring Boot 3.x
- **ML Libraries:**
  - DL4J (DeepLearning4J) for LSTM neural networks
  - XGBoost4J for gradient boosting
- **API:** REST endpoints for signal generation

### Frontend
- **Markup:** HTML5
- **Styling:** CSS3 (responsive design)
- **Scripting:** Vanilla JavaScript (ES6+)
- **Charts:** TradingView Lightweight Charts library
- **WebSocket:** Native WebSocket API

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                 WEB CLIENT (HTML + CSS + JavaScript)            │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────────────┐   │
│  │ Dashboard  │  │  TradingView │  │  Trading Controls    │   │
│  │ (Real-time)│  │  Chart       │  │                      │   │
│  └─────┬──────┘  └──────┬───────┘  └─────────┬────────────┘   │
└────────┼─────────────────┼──────────────────────┼───────────────┘
         │ WebSocket       │ AJAX/Fetch           │
         └─────────────────┼──────────────────────┘
                           │
┌──────────────────────────▼────────────────────────────────────┐
│                    PHP BACKEND (PHP 8.1+)                     │
│  ┌────────────┐  ┌──────────────┐  ┌─────────────┐          │
│  │    Auth    │  │   REST API   │  │  WebSocket  │          │
│  │ (Sessions) │  │   Router     │  │  Server     │          │
│  └─────┬──────┘  └──────┬───────┘  └──────┬──────┘          │
└────────┼─────────────────┼─────────────────┼──────────────────┘
         │                 │                 │
         ├─────────────────┴─────────────────┴──────────────┐
         │                                                   │
┌────────▼───────────────┐  ┌────────────────────────────────▼──┐
│  TRADING ENGINE (PHP)  │  │   ML SERVICE (Java + Spring Boot) │
│  ┌─────────────────┐   │  │  ┌──────────┐  ┌──────────────┐  │
│  │ Exchange Manager│◄──┼──┼──│  LSTM    │  │  XGBoost     │  │
│  │ (5 exchanges)   │   │  │  │  (DL4J)  │  │  (XGBoost4J) │  │
│  ├─────────────────┤   │  │  └──────────┘  └──────────────┘  │
│  │ Order Manager   │   │  │  ┌──────────────────────────────┐│
│  ├─────────────────┤   │  │  │   Feature Extractor          ││
│  │ Risk Manager    │   │  │  └──────────────────────────────┘│
│  ├─────────────────┤   │  └───────────────────────────────────┘
│  │ Strategy Engine │   │
│  ├─────────────────┤   │
│  │ Technical       │   │
│  │ Indicators      │   │
│  └─────────────────┘   │
└────────┬───────────────┘
         │
┌────────▼────────────────────────────────────────────────────────┐
│              MICROSOFT SQL SERVER 2012 DATABASE                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
│  │  users   │  │ exchanges│  │  orders  │  │  positions   │   │
│  ├──────────┤  ├──────────┤  ├──────────┤  ├──────────────┤   │
│  │  trades  │  │ backtest │  │  alerts  │  │  strategies  │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

## Project Structure

```
web-version-php/
├── backend/                  # PHP backend
│   ├── config/
│   │   ├── database.php     # MSSQL connection config
│   │   ├── config.php       # Application config
│   │   └── constants.php    # Constants
│   ├── src/
│   │   ├── api/             # REST API endpoints
│   │   │   ├── auth.php
│   │   │   ├── trading.php
│   │   │   ├── market.php
│   │   │   └── analysis.php
│   │   ├── auth/            # Authentication
│   │   │   ├── Auth.php
│   │   │   └── Session.php
│   │   ├── database/        # Database layer
│   │   │   ├── Connection.php
│   │   │   └── Models/
│   │   ├── exchanges/       # Exchange connectors
│   │   │   ├── BaseExchange.php
│   │   │   ├── BinanceExchange.php
│   │   │   ├── BybitExchange.php
│   │   │   └── ...
│   │   ├── trading/         # Trading logic
│   │   │   ├── OrderManager.php
│   │   │   ├── PositionManager.php
│   │   │   └── RiskManager.php
│   │   ├── indicators/      # Technical indicators
│   │   │   ├── EMA.php
│   │   │   ├── RSI.php
│   │   │   ├── MACD.php
│   │   │   └── BollingerBands.php
│   │   └── utils/           # Utilities
│   │       ├── Logger.php
│   │       └── Validator.php
│   ├── public/              # Web root
│   │   ├── index.php        # Entry point
│   │   └── websocket.php    # WebSocket server
│   └── composer.json        # PHP dependencies
├── ml-service/              # Java ML service
│   ├── src/main/java/com/cryptotrader/ml/
│   │   ├── MLServiceApplication.java
│   │   ├── controller/
│   │   │   └── SignalController.java
│   │   ├── service/
│   │   │   ├── LSTMService.java
│   │   │   ├── XGBoostService.java
│   │   │   └── FeatureExtractor.java
│   │   └── model/
│   │       └── SignalResponse.java
│   ├── src/main/resources/
│   │   ├── application.properties
│   │   └── models/          # Trained model files
│   ├── pom.xml              # Maven dependencies
│   └── Dockerfile
├── frontend/                # HTML/CSS/JS frontend
│   ├── index.html           # Main page
│   ├── login.html           # Login page
│   ├── dashboard.html       # Trading dashboard
│   ├── assets/
│   │   ├── css/
│   │   │   ├── main.css
│   │   │   ├── dashboard.css
│   │   │   └── responsive.css
│   │   └── js/
│   │       ├── app.js
│   │       ├── api.js
│   │       ├── chart.js
│   │       ├── trading.js
│   │       └── websocket.js
│   └── components/          # Reusable HTML components
├── database/                # Database scripts
│   ├── schema.sql           # MSSQL 2012 schema
│   ├── migrations/          # Migration scripts
│   └── seeds/               # Seed data
└── README.md
```

## Prerequisites

### PHP Requirements
- PHP 8.1 or higher
- Microsoft Drivers for PHP for SQL Server (php-sqlsrv, php-pdo_sqlsrv)
- php-mbstring
- php-json
- php-curl
- php-openssl
- Composer

### Java Requirements
- Java 17 or higher
- Maven 3.6+
- Spring Boot 3.x

### Database
- Microsoft SQL Server 2012 or higher
- SQL Server Management Studio (optional, for management)

### Web Server
- Apache 2.4+ with mod_rewrite or
- Nginx with PHP-FPM

## Installation

### 1. Install PHP Dependencies

```bash
cd backend
composer install
```

### 2. Configure Database Connection

Edit `backend/config/database.php`:

```php
return [
    'host' => 'localhost',
    'port' => 1433,
    'database' => 'cryptotrader',
    'username' => 'sa',
    'password' => 'your_password',
    'charset' => 'UTF-8'
];
```

### 3. Create Database Schema

```bash
# Using sqlcmd (SQL Server command-line tool)
sqlcmd -S localhost -U sa -P your_password -i database/schema.sql
```

### 4. Build and Run Java ML Service

```bash
cd ml-service
mvn clean package
java -jar target/ml-service-1.0.0.jar
```

The ML service will run on http://localhost:8080

### 5. Configure Web Server

#### Apache Configuration

Create `/etc/apache2/sites-available/cryptotrader.conf`:

```apache
<VirtualHost *:80>
    ServerName cryptotrader.local
    DocumentRoot /var/www/cryptotrader/backend/public

    <Directory /var/www/cryptotrader/backend/public>
        AllowOverride All
        Require all granted
    </Directory>

    Alias /frontend /var/www/cryptotrader/frontend
    <Directory /var/www/cryptotrader/frontend>
        Options Indexes FollowSymLinks
        AllowOverride None
        Require all granted
    </Directory>

    ErrorLog ${APACHE_LOG_DIR}/cryptotrader_error.log
    CustomLog ${APACHE_LOG_DIR}/cryptotrader_access.log combined
</VirtualHost>
```

Enable the site:
```bash
sudo a2ensite cryptotrader
sudo systemctl restart apache2
```

#### Nginx Configuration

Create `/etc/nginx/sites-available/cryptotrader`:

```nginx
server {
    listen 80;
    server_name cryptotrader.local;

    root /var/www/cryptotrader/backend/public;
    index index.php;

    location / {
        try_files $uri $uri/ /index.php?$query_string;
    }

    location /frontend {
        alias /var/www/cryptotrader/frontend;
        try_files $uri $uri/ =404;
    }

    location ~ \.php$ {
        fastcgi_pass unix:/run/php/php8.1-fpm.sock;
        fastcgi_index index.php;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
        include fastcgi_params;
    }
}
```

Enable the site:
```bash
sudo ln -s /etc/nginx/sites-available/cryptotrader /etc/nginx/sites-enabled/
sudo systemctl restart nginx
```

## Running

### Start ML Service
```bash
cd ml-service
java -jar target/ml-service-1.0.0.jar
```

### Start WebSocket Server (optional, for real-time updates)
```bash
cd backend
php public/websocket.php
```

### Access Application
Open browser and navigate to: http://cryptotrader.local

## Features

### Core Features
- ✅ Multi-exchange support (Binance, Bybit, OKX, Bitget, KuCoin)
- ✅ REST API integration with exchanges
- ✅ Real-time market data via WebSocket
- ✅ Technical indicators (EMA, RSI, ATR, MACD, Bollinger Bands)
- ✅ ML-enhanced signals (LSTM + XGBoost ensemble via Java service)
- ✅ Paper trading & live trading
- ✅ Position management (long/short)
- ✅ Risk management (Kelly criterion, trailing stops)
- ✅ Order management
- ✅ PnL tracking
- ✅ User authentication and session management
- ✅ Responsive HTML/CSS interface
- ✅ Real-time chart updates

### Security Features
- Session-based authentication
- API key encryption (AES-256)
- SQL injection prevention (parameterized queries)
- XSS protection (output escaping)
- CSRF tokens
- Rate limiting
- Input validation

## API Endpoints

### Authentication
- `POST /api/auth/login` - User login
- `POST /api/auth/logout` - User logout
- `POST /api/auth/register` - User registration

### Market Data
- `GET /api/market/ticker/:symbol` - Get ticker price
- `GET /api/market/candles/:symbol` - Get historical candles
- `GET /api/market/orderbook/:symbol` - Get order book

### Trading
- `POST /api/trading/order` - Place order
- `GET /api/trading/orders` - Get open orders
- `POST /api/trading/cancel/:orderId` - Cancel order
- `GET /api/trading/positions` - Get open positions
- `GET /api/trading/history` - Get trade history

### Analysis
- `GET /api/analysis/indicators/:symbol` - Get technical indicators
- `POST /api/analysis/signal` - Get ML-enhanced trading signal
- `GET /api/analysis/backtest` - Run backtest

## Configuration

Edit `backend/config/config.php`:

```php
return [
    'app' => [
        'name' => 'CryptoTrader',
        'debug' => false,
        'timezone' => 'UTC'
    ],
    'session' => [
        'lifetime' => 7200,
        'secure' => true,
        'httponly' => true
    ],
    'ml_service' => [
        'url' => 'http://localhost:8080',
        'timeout' => 30
    ],
    'exchanges' => [
        'binance' => [
            'testnet' => true,
            'base_url' => 'https://testnet.binance.vision'
        ]
    ],
    'trading' => [
        'default_mode' => 'paper',
        'default_symbol' => 'BTCUSDT',
        'default_interval' => '15m'
    ]
];
```

## Database Schema

The application uses Microsoft SQL Server 2012 with the following tables:

- `users` - User accounts
- `exchanges` - Exchange API credentials (encrypted)
- `orders` - Order records
- `positions` - Position records
- `trades` - Trade history
- `strategies` - Trading strategies
- `alerts` - Price alerts
- `backtest_results` - Backtesting results

See `database/schema.sql` for complete schema definition.

## Development

### Running Tests

PHP tests:
```bash
cd backend
composer test
```

Java tests:
```bash
cd ml-service
mvn test
```

### Code Style

PHP follows PSR-12 coding standard:
```bash
composer phpcs
```

Java follows Google Java Style Guide:
```bash
mvn checkstyle:check
```

## Deployment

### Production Checklist
- [ ] Set `debug => false` in config.php
- [ ] Use HTTPS/SSL certificates
- [ ] Enable PHP OPcache
- [ ] Configure proper file permissions
- [ ] Set up database backups
- [ ] Enable error logging (not display)
- [ ] Use environment variables for sensitive data
- [ ] Configure firewall rules
- [ ] Set up monitoring and alerts

## License

Same as the original CryptoTrader C++ version.

## Support

For issues, please open a GitHub issue in the main repository.
