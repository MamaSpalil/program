# CryptoTrader Web Version

Web-based cryptocurrency trading platform with ML-enhanced signals, based on the original C++ desktop application.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    WEB CLIENT (React + TypeScript)              │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────────────┐   │
│  │ Dashboard  │  │  TradingView │  │  Strategy Builder    │   │
│  │ (Real-time)│  │  Chart       │  │  (Visual Editor)     │   │
│  └─────┬──────┘  └──────┬───────┘  └─────────┬────────────┘   │
└────────┼─────────────────┼──────────────────────┼───────────────┘
         │ WebSocket       │ REST API             │
         └─────────────────┼──────────────────────┘
                           │
┌──────────────────────────▼────────────────────────────────────┐
│              API GATEWAY (Node.js + Express + TypeScript)     │
│  ┌────────────┐  ┌──────────┐  ┌─────────────┐              │
│  │    Auth    │  │  Router  │  │  WebSocket  │              │
│  │  (JWT)     │  │          │  │  Manager    │              │
│  └─────┬──────┘  └────┬─────┘  └──────┬──────┘              │
└────────┼──────────────┼────────────────┼─────────────────────┘
         │              │                │
         ├──────────────┴────────────────┴──────────────┐
         │                                               │
┌────────▼───────────────┐  ┌────────────────────────────▼──────┐
│  TRADING SERVICE       │  │   ML SERVICE (Python + FastAPI)   │
│  (Node.js/TypeScript)  │  │  ┌──────────┐  ┌──────────────┐  │
│  ┌─────────────────┐   │  │  │  LSTM    │  │  XGBoost     │  │
│  │ Exchange Manager│◄──┼──┼──│  Model   │  │  Model       │  │
│  │ (5 exchanges)   │   │  │  └──────────┘  └──────────────┘  │
│  ├─────────────────┤   │  │  ┌──────────────────────────────┐│
│  │ Order Manager   │   │  │  │   Feature Extractor          ││
│  ├─────────────────┤   │  │  └──────────────────────────────┘│
│  │ Risk Manager    │   │  └───────────────────────────────────┘
│  ├─────────────────┤   │
│  │ Strategy Engine │   │  ┌────────────────────────────────────┐
│  ├─────────────────┤   │  │  REDIS (Cache & Pub/Sub)          │
│  │ Backtest Engine │   │  │  - Market data cache              │
│  └─────────────────┘   │  │  - WebSocket pub/sub              │
└────────┬───────────────┘  │  - Rate limiting                  │
         │                  └────────────────────────────────────┘
         │
┌────────▼────────────────────────────────────────────────────────┐
│                    PostgreSQL DATABASE                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
│  │  Users   │  │ Exchanges│  │  Orders  │  │  Positions   │   │
│  ├──────────┤  ├──────────┤  ├──────────┤  ├──────────────┤   │
│  │  Trades  │  │ Backtest │  │  Alerts  │  │  Strategies  │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

## Technology Stack

### Backend
- **Runtime:** Node.js 20+ with TypeScript
- **Framework:** Express.js
- **Database:** PostgreSQL 15+
- **Cache:** Redis 7+
- **WebSocket:** Socket.io
- **ORM:** Prisma
- **Authentication:** JWT (jsonwebtoken)
- **Validation:** Zod
- **Testing:** Jest

### ML Service
- **Runtime:** Python 3.11+
- **Framework:** FastAPI
- **ML Libraries:** PyTorch, XGBoost, scikit-learn
- **Data Processing:** pandas, numpy

### Frontend
- **Framework:** React 18+ with TypeScript
- **State:** Redux Toolkit
- **Charts:** TradingView Lightweight Charts
- **UI:** Material-UI (MUI)
- **WebSocket:** Socket.io-client
- **Build:** Vite

## Project Structure

```
web-version/
├── backend/              # Node.js backend API
│   ├── src/
│   │   ├── auth/        # Authentication & JWT
│   │   ├── exchanges/   # Exchange connectors (5 exchanges)
│   │   ├── trading/     # Order/position management
│   │   ├── indicators/  # Technical indicators
│   │   ├── strategies/  # Trading strategies
│   │   ├── risk/        # Risk management
│   │   ├── backtest/    # Backtesting engine
│   │   ├── websocket/   # Real-time data streaming
│   │   ├── api/         # REST API routes
│   │   └── utils/       # Utilities
│   ├── prisma/          # Database schema & migrations
│   ├── tests/           # Unit & integration tests
│   ├── package.json
│   └── tsconfig.json
├── ml-service/          # Python ML service
│   ├── app/
│   │   ├── models/      # LSTM, XGBoost models
│   │   ├── features/    # Feature extraction
│   │   ├── training/    # Model training pipelines
│   │   └── api/         # FastAPI endpoints
│   ├── requirements.txt
│   └── Dockerfile
├── frontend/            # React frontend
│   ├── src/
│   │   ├── components/  # React components
│   │   ├── pages/       # Page components
│   │   ├── store/       # Redux store
│   │   ├── hooks/       # Custom hooks
│   │   ├── services/    # API clients
│   │   └── utils/       # Utilities
│   ├── public/
│   ├── package.json
│   └── tsconfig.json
├── database/            # Database related files
│   ├── schema.sql       # PostgreSQL schema
│   ├── migrations/      # SQL migrations
│   └── seeds/           # Seed data
├── docker/              # Docker configuration
│   ├── docker-compose.yml
│   ├── backend.Dockerfile
│   ├── ml-service.Dockerfile
│   └── frontend.Dockerfile
└── README.md
```

## Features

### Core Features (from C++ version)
- ✅ Multi-exchange support (Binance, Bybit, OKX, Bitget, KuCoin)
- ✅ REST API & WebSocket integration
- ✅ Real-time market data streaming
- ✅ Technical indicators (EMA, RSI, ATR, MACD, Bollinger Bands)
- ✅ ML-enhanced signals (LSTM + XGBoost ensemble)
- ✅ Paper trading & live trading
- ✅ Position management (long/short)
- ✅ Risk management (Kelly criterion, trailing stops)
- ✅ Backtesting engine
- ✅ Order management
- ✅ PnL tracking
- ✅ Alerts & notifications

### Web-Specific Features
- 🆕 Multi-user support with authentication
- 🆕 Cloud-based storage
- 🆕 Responsive web interface
- 🆕 Mobile-friendly design
- 🆕 Centralized ML model training
- 🆕 Social features (strategy sharing)
- 🆕 Advanced analytics dashboard
- 🆕 API rate limiting per user
- 🆕 Audit logs
- 🆕 Two-factor authentication (2FA)

## Getting Started

### Prerequisites

- Node.js 20+
- Python 3.11+
- PostgreSQL 15+
- Redis 7+
- Docker & Docker Compose (optional)

### Installation

1. Clone the repository
```bash
git clone https://github.com/MamaSpalil/program.git
cd program/web-version
```

2. Install backend dependencies
```bash
cd backend
npm install
```

3. Install ML service dependencies
```bash
cd ../ml-service
pip install -r requirements.txt
```

4. Install frontend dependencies
```bash
cd ../frontend
npm install
```

5. Set up environment variables
```bash
cp backend/.env.example backend/.env
cp ml-service/.env.example ml-service/.env
# Edit .env files with your configuration
```

6. Set up database
```bash
cd backend
npx prisma migrate dev
npx prisma db seed
```

### Running with Docker

```bash
docker-compose up -d
```

Services will be available at:
- Frontend: http://localhost:3000
- Backend API: http://localhost:4000
- ML Service: http://localhost:8000
- PostgreSQL: localhost:5432
- Redis: localhost:6379

### Running Locally

Terminal 1 - Backend:
```bash
cd backend
npm run dev
```

Terminal 2 - ML Service:
```bash
cd ml-service
uvicorn app.main:app --reload --port 8000
```

Terminal 3 - Frontend:
```bash
cd frontend
npm run dev
```

## Configuration

Edit `backend/.env`:

```env
# Database
DATABASE_URL=postgresql://user:password@localhost:5432/cryptotrader
REDIS_URL=redis://localhost:6379

# JWT
JWT_SECRET=your-secret-key-here
JWT_EXPIRES_IN=7d

# ML Service
ML_SERVICE_URL=http://localhost:8000

# Exchanges (per user in database)
# These are defaults for testing
BINANCE_TESTNET=true
```

## API Documentation

Once running, visit:
- Backend API docs: http://localhost:4000/api-docs
- ML Service docs: http://localhost:8000/docs

## Testing

Backend tests:
```bash
cd backend
npm test
```

ML service tests:
```bash
cd ml-service
pytest
```

Frontend tests:
```bash
cd frontend
npm test
```

## Security

- All API keys are encrypted (AES-256) before storage
- JWT authentication for all endpoints
- HTTPS/TLS in production
- Rate limiting per user
- SQL injection prevention via Prisma
- XSS protection
- CSRF tokens
- Input validation with Zod

## Deployment

### Production Build

Backend:
```bash
cd backend
npm run build
npm start
```

Frontend:
```bash
cd frontend
npm run build
# Serve dist/ folder with nginx or similar
```

### Docker Production

```bash
docker-compose -f docker-compose.prod.yml up -d
```

## Bug Fixes from C++ Version

All 101 issues from the original C++ codebase have been addressed:
- ✅ Correct placeOrder implementation for all exchanges
- ✅ Proper SHORT position equity calculation
- ✅ Thread-safe operations
- ✅ Bounds checking on all array accesses
- ✅ Memory leak fixes (N/A in Node.js with garbage collection)
- ✅ Division by zero protection
- ✅ Proper WebSocket symbol/interval handling
- ✅ Rate limiting implementation
- ✅ Security improvements (encrypted API keys, HTTPS)

## License

Same as the original CryptoTrader C++ version.

## Support

For issues, please open a GitHub issue in the main repository.
