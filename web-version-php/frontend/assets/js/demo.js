// Demo Mode JavaScript - Standalone version with mock data
// No backend required - all data is simulated

// ========== Mock Data Generators ==========

// Symbol prices (base prices for simulation)
const symbolPrices = {
    'BTCUSDT': 65000,
    'ETHUSDT': 3200,
    'BNBUSDT': 580,
    'SOLUSDT': 150,
    'ADAUSDT': 0.45
};

// Current state
let currentSymbol = 'BTCUSDT';
let currentInterval = '15m';
let chart = null;
let candlestickSeries = null;
let updateInterval = null;
let demoPositions = [];

// Generate realistic candle data
function generateCandleData(basePrice, count = 200) {
    const candles = [];
    const now = Math.floor(Date.now() / 1000);
    let price = basePrice;

    // Generate historical candles
    for (let i = count; i >= 0; i--) {
        const timestamp = now - (i * 15 * 60); // 15 minute intervals

        // Add some volatility
        const volatility = basePrice * 0.002; // 0.2% volatility
        const change = (Math.random() - 0.5) * volatility;
        price = price + change;

        // Generate OHLC
        const open = price;
        const high = price + Math.random() * volatility;
        const low = price - Math.random() * volatility;
        const close = price + (Math.random() - 0.5) * volatility;

        candles.push({
            time: timestamp,
            open: parseFloat(open.toFixed(2)),
            high: parseFloat(high.toFixed(2)),
            low: parseFloat(low.toFixed(2)),
            close: parseFloat(close.toFixed(2))
        });

        price = close; // Next candle starts from current close
    }

    return candles;
}

// Calculate RSI
function calculateRSI(candles, period = 14) {
    if (candles.length < period + 1) return 50;

    let gains = 0;
    let losses = 0;

    for (let i = candles.length - period; i < candles.length; i++) {
        const change = candles[i].close - candles[i - 1].close;
        if (change > 0) gains += change;
        else losses += Math.abs(change);
    }

    const avgGain = gains / period;
    const avgLoss = losses / period;

    if (avgLoss === 0) return 100;

    const rs = avgGain / avgLoss;
    const rsi = 100 - (100 / (1 + rs));

    return rsi;
}

// Calculate EMA
function calculateEMA(candles, period = 21) {
    if (candles.length < period) return candles[candles.length - 1].close;

    const multiplier = 2 / (period + 1);
    let ema = candles[candles.length - period].close;

    for (let i = candles.length - period + 1; i < candles.length; i++) {
        ema = (candles[i].close - ema) * multiplier + ema;
    }

    return ema;
}

// Calculate MACD
function calculateMACD(candles) {
    if (candles.length < 26) return 0;

    const ema12 = calculateEMA(candles, 12);
    const ema26 = calculateEMA(candles, 26);

    return ema12 - ema26;
}

// Calculate ATR
function calculateATR(candles, period = 14) {
    if (candles.length < period + 1) return 0;

    let atr = 0;

    for (let i = candles.length - period; i < candles.length; i++) {
        const high = candles[i].high;
        const low = candles[i].low;
        const prevClose = candles[i - 1].close;

        const tr = Math.max(
            high - low,
            Math.abs(high - prevClose),
            Math.abs(low - prevClose)
        );

        atr += tr;
    }

    return atr / period;
}

// Generate ML signal based on indicators
function generateMLSignal(rsi, macd, candles) {
    const currentPrice = candles[candles.length - 1].close;
    const ema21 = calculateEMA(candles, 21);

    let signal = 'NEUTRAL';
    let confidence = 0;

    // Simple strategy: RSI oversold/overbought + price vs EMA + MACD
    if (rsi < 35 && currentPrice < ema21 && macd < 0) {
        signal = 'BUY';
        confidence = Math.min(95, 60 + Math.random() * 20);
    } else if (rsi > 65 && currentPrice > ema21 && macd > 0) {
        signal = 'SELL';
        confidence = Math.min(95, 60 + Math.random() * 20);
    } else if (rsi < 45 && macd > 0) {
        signal = 'BUY';
        confidence = Math.min(80, 50 + Math.random() * 20);
    } else if (rsi > 55 && macd < 0) {
        signal = 'SELL';
        confidence = Math.min(80, 50 + Math.random() * 20);
    } else {
        signal = 'NEUTRAL';
        confidence = 40 + Math.random() * 20;
    }

    return { signal, confidence: confidence.toFixed(1) };
}

// ========== Chart Initialization ==========

function initChart() {
    const chartContainer = document.getElementById('tradingChart');

    if (!chartContainer) {
        console.error('Chart container not found');
        return;
    }

    chart = LightweightCharts.createChart(chartContainer, {
        width: chartContainer.clientWidth,
        height: chartContainer.clientHeight,
        layout: {
            background: { color: '#1a1a2e' },
            textColor: '#b0b0b0'
        },
        grid: {
            vertLines: { color: '#2d3748' },
            horzLines: { color: '#2d3748' }
        },
        crosshair: {
            mode: LightweightCharts.CrosshairMode.Normal
        },
        timeScale: {
            timeVisible: true,
            secondsVisible: false
        },
        rightPriceScale: {
            borderColor: '#2d3748'
        }
    });

    candlestickSeries = chart.addCandlestickSeries({
        upColor: '#28a745',
        downColor: '#dc3545',
        borderUpColor: '#28a745',
        borderDownColor: '#dc3545',
        wickUpColor: '#28a745',
        wickDownColor: '#dc3545'
    });

    // Handle resize
    window.addEventListener('resize', () => {
        if (chart && chartContainer) {
            chart.applyOptions({
                width: chartContainer.clientWidth
            });
        }
    });
}

// ========== UI Update Functions ==========

function updateUI() {
    const basePrice = symbolPrices[currentSymbol];
    const candles = generateCandleData(basePrice);

    // Update chart
    if (candlestickSeries) {
        candlestickSeries.setData(candles);
        chart.timeScale().fitContent();
    }

    // Get current price (last candle close)
    const currentPrice = candles[candles.length - 1].close;
    const prevPrice = candles[candles.length - 2].close;
    const priceChange = ((currentPrice - prevPrice) / prevPrice) * 100;

    // Update price info
    document.getElementById('currentPrice').textContent = `$${currentPrice.toFixed(2)}`;

    const priceChangeElement = document.getElementById('priceChange');
    priceChangeElement.textContent = `${priceChange >= 0 ? '+' : ''}${priceChange.toFixed(2)}%`;
    priceChangeElement.className = `info-value ${priceChange >= 0 ? 'text-success' : 'text-danger'}`;

    // Generate volume (mock)
    const volume = (Math.random() * 500 + 100).toFixed(2);
    document.getElementById('volume').textContent = `${volume}M`;

    // Calculate and display indicators
    const rsi = calculateRSI(candles);
    const rsiElement = document.getElementById('rsiValue');
    rsiElement.textContent = rsi.toFixed(2);
    rsiElement.className = 'indicator-value';
    if (rsi < 30) rsiElement.classList.add('text-success');
    else if (rsi > 70) rsiElement.classList.add('text-danger');

    const macd = calculateMACD(candles);
    const macdElement = document.getElementById('macdValue');
    macdElement.textContent = macd.toFixed(4);
    macdElement.className = 'indicator-value';
    if (macd > 0) macdElement.classList.add('text-success');
    else macdElement.classList.add('text-danger');

    const ema = calculateEMA(candles, 21);
    document.getElementById('emaValue').textContent = `$${ema.toFixed(2)}`;

    const atr = calculateATR(candles);
    document.getElementById('atrValue').textContent = atr.toFixed(4);

    // Generate and display ML signal
    const mlSignal = generateMLSignal(rsi, macd, candles);
    const signalElement = document.getElementById('mlSignal');
    signalElement.textContent = `${mlSignal.signal} (${mlSignal.confidence}%)`;
    signalElement.className = 'info-value';

    if (mlSignal.signal === 'BUY') {
        signalElement.classList.add('text-success');
    } else if (mlSignal.signal === 'SELL') {
        signalElement.classList.add('text-danger');
    } else {
        signalElement.classList.add('text-warning');
    }
}

function updatePositions() {
    const positionsList = document.getElementById('positionsList');

    if (demoPositions.length === 0) {
        positionsList.innerHTML = '<p class="empty-state">No open positions</p>';
        return;
    }

    const basePrice = symbolPrices[currentSymbol];
    const currentPrice = basePrice + (Math.random() - 0.5) * basePrice * 0.01;

    const positionsHTML = demoPositions.map(pos => {
        const pnl = (currentPrice - pos.entry_price) * pos.quantity * (pos.side === 'LONG' ? 1 : -1);
        const pnlPercent = ((currentPrice - pos.entry_price) / pos.entry_price) * 100 * (pos.side === 'LONG' ? 1 : -1);

        return `
            <div class="position-item">
                <div class="position-info">
                    <div class="position-symbol">${pos.symbol} - ${pos.side}</div>
                    <div class="position-details">
                        Entry: $${pos.entry_price.toFixed(2)} | Qty: ${pos.quantity} | PnL: ${pnlPercent.toFixed(2)}%
                    </div>
                </div>
                <div class="position-pnl ${pnl >= 0 ? 'text-success' : 'text-danger'}">
                    ${pnl >= 0 ? '+' : ''}$${pnl.toFixed(2)}
                </div>
            </div>
        `;
    }).join('');

    positionsList.innerHTML = positionsHTML;
}

// ========== Event Handlers ==========

document.getElementById('symbolSelect').addEventListener('change', (e) => {
    currentSymbol = e.target.value;
    updateUI();
});

document.getElementById('intervalSelect').addEventListener('change', (e) => {
    currentInterval = e.target.value;
    updateUI();
});

document.getElementById('buyBtn').addEventListener('click', () => {
    const quantity = parseFloat(document.getElementById('quantity').value);
    const basePrice = symbolPrices[currentSymbol];
    const entryPrice = basePrice + (Math.random() - 0.5) * basePrice * 0.002;

    if (quantity > 0) {
        demoPositions.push({
            symbol: currentSymbol,
            side: 'LONG',
            entry_price: entryPrice,
            quantity: quantity,
            timestamp: Date.now()
        });

        updatePositions();

        // Show success message
        showNotification('BUY order executed successfully!', 'success');
    } else {
        showNotification('Please enter a valid quantity', 'error');
    }
});

document.getElementById('sellBtn').addEventListener('click', () => {
    const quantity = parseFloat(document.getElementById('quantity').value);
    const basePrice = symbolPrices[currentSymbol];
    const entryPrice = basePrice + (Math.random() - 0.5) * basePrice * 0.002;

    if (quantity > 0) {
        demoPositions.push({
            symbol: currentSymbol,
            side: 'SHORT',
            entry_price: entryPrice,
            quantity: quantity,
            timestamp: Date.now()
        });

        updatePositions();

        // Show success message
        showNotification('SELL order executed successfully!', 'success');
    } else {
        showNotification('Please enter a valid quantity', 'error');
    }
});

document.getElementById('infoBtn').addEventListener('click', () => {
    showNotification(
        '📊 This is a DEMO version with simulated data.\n\n' +
        '✨ Features:\n' +
        '• Real-time chart visualization\n' +
        '• Technical indicators (RSI, MACD, EMA, ATR)\n' +
        '• ML-enhanced trading signals\n' +
        '• Interactive trading simulation\n\n' +
        '🔗 Full version requires PHP backend + Java ML service\n' +
        '📖 See README.md for installation instructions',
        'info',
        8000
    );
});

// Notification system
function showNotification(message, type = 'info', duration = 3000) {
    // Remove existing notifications
    const existing = document.querySelector('.notification');
    if (existing) {
        existing.remove();
    }

    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.style.cssText = `
        position: fixed;
        top: 80px;
        right: 20px;
        background: ${type === 'success' ? '#28a745' : type === 'error' ? '#dc3545' : '#0066ff'};
        color: white;
        padding: 1rem 1.5rem;
        border-radius: 8px;
        box-shadow: 0 4px 12px rgba(0,0,0,0.3);
        z-index: 10000;
        max-width: 400px;
        white-space: pre-line;
        animation: slideIn 0.3s ease-out;
    `;

    notification.textContent = message;
    document.body.appendChild(notification);

    setTimeout(() => {
        notification.style.animation = 'slideOut 0.3s ease-out';
        setTimeout(() => notification.remove(), 300);
    }, duration);
}

// Add CSS animations
const style = document.createElement('style');
style.textContent = `
    @keyframes slideIn {
        from {
            transform: translateX(400px);
            opacity: 0;
        }
        to {
            transform: translateX(0);
            opacity: 1;
        }
    }

    @keyframes slideOut {
        from {
            transform: translateX(0);
            opacity: 1;
        }
        to {
            transform: translateX(400px);
            opacity: 0;
        }
    }
`;
document.head.appendChild(style);

// ========== Initialization ==========

// Initialize chart
initChart();

// Initial data load
updateUI();

// Auto-update every 5 seconds
updateInterval = setInterval(() => {
    // Update with slight variations for live feel
    updateUI();
    updatePositions();
}, 5000);

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    if (updateInterval) {
        clearInterval(updateInterval);
    }
});

// Show welcome message
setTimeout(() => {
    showNotification('🎯 Welcome to CryptoTrader Demo!\nExplore the features with simulated data.', 'info', 4000);
}, 500);
