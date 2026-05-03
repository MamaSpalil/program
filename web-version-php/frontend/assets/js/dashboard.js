// Dashboard JavaScript

// Check authentication
if (!sessionStorage.getItem('user_id')) {
    window.location.href = 'index.html';
}

// Display user info
const username = sessionStorage.getItem('username');
document.getElementById('userDisplay').textContent = `Welcome, ${username}`;

// Logout handler
document.getElementById('logoutBtn').addEventListener('click', async () => {
    try {
        await api.logout();
        sessionStorage.clear();
        window.location.href = 'index.html';
    } catch (error) {
        console.error('Logout failed:', error);
    }
});

// Current state
let currentSymbol = 'BTCUSDT';
let currentInterval = '15m';
let updateInterval = null;

// Symbol and interval change handlers
document.getElementById('symbolSelect').addEventListener('change', (e) => {
    currentSymbol = e.target.value;
    updateData();
});

document.getElementById('intervalSelect').addEventListener('change', (e) => {
    currentInterval = e.target.value;
    updateData();
});

// Update market data
async function updateData() {
    try {
        // Get ticker data
        const ticker = await api.getTicker(currentSymbol);
        if (ticker.success) {
            const data = ticker.data;
            document.getElementById('currentPrice').textContent = `$${parseFloat(data.price).toFixed(2)}`;

            const change = parseFloat(data.priceChangePercent);
            const changeElement = document.getElementById('priceChange');
            changeElement.textContent = `${change >= 0 ? '+' : ''}${change.toFixed(2)}%`;
            changeElement.className = `info-value ${change >= 0 ? 'text-success' : 'text-danger'}`;

            document.getElementById('volume').textContent = `${(parseFloat(data.volume) / 1000000).toFixed(2)}M`;
        }

        // Get indicators
        const indicators = await api.getIndicators(currentSymbol, currentInterval);
        if (indicators.success) {
            const data = indicators.data;

            if (data.rsi) {
                const rsi = data.rsi[data.rsi.length - 1];
                const rsiElement = document.getElementById('rsiValue');
                rsiElement.textContent = rsi.toFixed(2);
                rsiElement.className = 'indicator-value';
                if (rsi < 30) rsiElement.classList.add('text-success');
                else if (rsi > 70) rsiElement.classList.add('text-danger');
            }

            if (data.macd) {
                const macd = data.macd[data.macd.length - 1];
                document.getElementById('macdValue').textContent = macd.toFixed(4);
            }

            if (data.ema_21) {
                const ema = data.ema_21[data.ema_21.length - 1];
                document.getElementById('emaValue').textContent = `$${ema.toFixed(2)}`;
            }

            if (data.atr) {
                const atr = data.atr[data.atr.length - 1];
                document.getElementById('atrValue').textContent = atr.toFixed(4);
            }
        }

        // Get ML signal
        const candles = await api.getCandles(currentSymbol, currentInterval, 100);
        if (candles.success && indicators.success) {
            const signal = await api.getMLSignal(
                currentSymbol,
                currentInterval,
                candles.data,
                indicators.data
            );

            if (signal.success) {
                const signalData = signal.data;
                const signalElement = document.getElementById('mlSignal');
                signalElement.textContent = signalData.signal;
                signalElement.className = 'info-value';

                if (signalData.signal === 'BUY') {
                    signalElement.classList.add('text-success');
                } else if (signalData.signal === 'SELL') {
                    signalElement.classList.add('text-danger');
                }
            }
        }

        // Update positions
        await updatePositions();

    } catch (error) {
        console.error('Failed to update data:', error);
    }
}

// Update positions list
async function updatePositions() {
    try {
        const positions = await api.getPositions();
        if (positions.success && positions.data.length > 0) {
            const positionsHTML = positions.data.map(pos => `
                <div class="position-item">
                    <div class="position-info">
                        <div class="position-symbol">${pos.symbol} - ${pos.side}</div>
                        <div class="position-details">
                            Entry: $${pos.entry_price} | Qty: ${pos.quantity}
                        </div>
                    </div>
                    <div class="position-pnl ${pos.unrealized_pnl >= 0 ? 'text-success' : 'text-danger'}">
                        ${pos.unrealized_pnl >= 0 ? '+' : ''}$${pos.unrealized_pnl.toFixed(2)}
                    </div>
                </div>
            `).join('');

            document.getElementById('positionsList').innerHTML = positionsHTML;
        } else {
            document.getElementById('positionsList').innerHTML = '<p class="empty-state">No open positions</p>';
        }
    } catch (error) {
        console.error('Failed to fetch positions:', error);
    }
}

// Initial data load
updateData();

// Auto-update every 5 seconds
updateInterval = setInterval(updateData, 5000);

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    if (updateInterval) {
        clearInterval(updateInterval);
    }
});
