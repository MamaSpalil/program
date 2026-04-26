// Trading Functions

// Buy button handler
document.getElementById('buyBtn')?.addEventListener('click', async () => {
    await placeTrade('BUY');
});

// Sell button handler
document.getElementById('sellBtn')?.addEventListener('click', async () => {
    await placeTrade('SELL');
});

// Place trade function
async function placeTrade(side) {
    const quantity = parseFloat(document.getElementById('quantity').value);
    const orderType = document.getElementById('orderType').value;

    if (!quantity || quantity <= 0) {
        alert('Please enter a valid quantity');
        return;
    }

    try {
        const orderData = {
            symbol: currentSymbol,
            side: side,
            type: orderType.toUpperCase(),
            quantity: quantity
        };

        // Add price for limit orders
        if (orderType === 'limit') {
            const priceInput = prompt('Enter limit price:');
            if (!priceInput) return;

            orderData.price = parseFloat(priceInput);
            if (isNaN(orderData.price) || orderData.price <= 0) {
                alert('Invalid price');
                return;
            }
        }

        const response = await api.placeOrder(orderData);

        if (response.success) {
            alert(`${side} order placed successfully!`);
            updatePositions();
        } else {
            alert(`Order failed: ${response.error}`);
        }
    } catch (error) {
        alert(`Failed to place order: ${error.message}`);
        console.error('Place order error:', error);
    }
}

// Calculate position size based on risk
function calculatePositionSize(accountBalance, riskPercent, stopLossPercent) {
    const riskAmount = accountBalance * (riskPercent / 100);
    return riskAmount / (stopLossPercent / 100);
}

// Format number with commas
function formatNumber(num, decimals = 2) {
    return num.toFixed(decimals).replace(/\B(?=(\d{3})+(?!\d))/g, ',');
}

// Format currency
function formatCurrency(num, decimals = 2) {
    return '$' + formatNumber(num, decimals);
}

// Calculate PnL percentage
function calculatePnLPercent(entryPrice, currentPrice, side) {
    if (side === 'LONG' || side === 'BUY') {
        return ((currentPrice - entryPrice) / entryPrice) * 100;
    } else {
        return ((entryPrice - currentPrice) / entryPrice) * 100;
    }
}
