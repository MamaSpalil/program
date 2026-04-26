// Chart Management using TradingView Lightweight Charts

let chart = null;
let candlestickSeries = null;

// Initialize chart
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

// Update chart with candle data
async function updateChart(symbol, interval) {
    try {
        const response = await api.getCandles(symbol, interval, 200);

        if (response.success && response.data) {
            const candles = response.data.map(candle => ({
                time: candle[0] / 1000, // Convert to seconds
                open: candle[1],
                high: candle[2],
                low: candle[3],
                close: candle[4]
            }));

            if (candlestickSeries) {
                candlestickSeries.setData(candles);
                chart.timeScale().fitContent();
            }
        }
    } catch (error) {
        console.error('Failed to update chart:', error);
    }
}

// Initialize chart on page load
if (document.getElementById('tradingChart')) {
    initChart();

    // Initial chart load
    updateChart(currentSymbol, currentInterval);

    // Update chart when symbol or interval changes
    document.getElementById('symbolSelect').addEventListener('change', (e) => {
        updateChart(e.target.value, currentInterval);
    });

    document.getElementById('intervalSelect').addEventListener('change', (e) => {
        updateChart(currentSymbol, e.target.value);
    });
}
