// API Client for CryptoTrader

const API_BASE_URL = '';

class APIClient {
    constructor() {
        this.baseURL = API_BASE_URL;
    }

    async request(endpoint, options = {}) {
        const url = `${this.baseURL}${endpoint}`;
        const defaultOptions = {
            headers: {
                'Content-Type': 'application/json'
            }
        };

        const config = { ...defaultOptions, ...options };

        try {
            const response = await fetch(url, config);
            const data = await response.json();

            if (!response.ok) {
                throw new Error(data.error || 'API request failed');
            }

            return data;
        } catch (error) {
            console.error('API Error:', error);
            throw error;
        }
    }

    // Auth endpoints
    async login(username, password, rememberMe = false) {
        return this.request('/api/auth/login', {
            method: 'POST',
            body: JSON.stringify({ username, password, remember_me: rememberMe })
        });
    }

    async logout() {
        return this.request('/api/auth/logout', {
            method: 'POST'
        });
    }

    async getCurrentUser() {
        return this.request('/api/auth/me');
    }

    // Market endpoints
    async getTicker(symbol) {
        return this.request(`/api/market/ticker/${symbol}`);
    }

    async getCandles(symbol, interval, limit = 100) {
        return this.request(`/api/market/candles/${symbol}?interval=${interval}&limit=${limit}`);
    }

    // Trading endpoints
    async placeOrder(orderData) {
        return this.request('/api/trading/order', {
            method: 'POST',
            body: JSON.stringify(orderData)
        });
    }

    async getPositions() {
        return this.request('/api/trading/positions');
    }

    async getOpenOrders() {
        return this.request('/api/trading/orders');
    }

    // Analysis endpoints
    async getIndicators(symbol, interval) {
        return this.request(`/api/analysis/indicators/${symbol}?interval=${interval}`);
    }

    async getMLSignal(symbol, interval, candles, indicators) {
        return this.request('/api/analysis/signal', {
            method: 'POST',
            body: JSON.stringify({ symbol, interval, candles, indicators })
        });
    }
}

// Export singleton instance
const api = new APIClient();
