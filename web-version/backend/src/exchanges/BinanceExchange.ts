import { BaseExchange, ExchangeConfig, OrderRequest, OrderResponse, Position, Candle } from './BaseExchange';
import { AxiosRequestConfig } from 'axios';

export class BinanceExchange extends BaseExchange {
  constructor(config: ExchangeConfig) {
    super('Binance', config);

    // Set URLs based on testnet flag
    if (config.testnet) {
      this.baseUrl = 'https://testnet.binancefuture.com';
      this.wsUrl = 'wss://stream.binancefuture.com';
    } else {
      this.baseUrl = 'https://fapi.binance.com';
      this.wsUrl = 'wss://fstream.binance.com';
    }
  }

  protected signRequest(
    config: AxiosRequestConfig,
    method: string,
    endpoint: string,
    params: any
  ): void {
    const timestamp = Date.now();
    const queryString = new URLSearchParams({
      ...params,
      timestamp: timestamp.toString(),
    }).toString();

    const signature = this.createSignature(this.config.apiSecret, queryString);

    config.headers = {
      ...config.headers,
      'X-MBX-APIKEY': this.config.apiKey,
    };

    if (method === 'GET' || method === 'DELETE') {
      config.params = {
        ...params,
        timestamp,
        signature,
      };
    } else {
      config.data = {
        ...params,
        timestamp,
        signature,
      };
    }
  }

  async getPrice(symbol: string): Promise<number> {
    const data = await this.httpGet('/fapi/v1/ticker/price', { symbol });

    if (!data || !data.price) {
      throw new Error(`Invalid price data for ${symbol}`);
    }

    return this.safeStod(data.price);
  }

  async getKlines(symbol: string, interval: string, limit: number = 500): Promise<Candle[]> {
    const data = await this.httpGet('/fapi/v1/klines', {
      symbol,
      interval,
      limit,
    });

    if (!Array.isArray(data)) {
      throw new Error('Invalid klines data');
    }

    return data.map((k: any[]) => ({
      timestamp: k[0],
      open: this.safeStod(k[1]),
      high: this.safeStod(k[2]),
      low: this.safeStod(k[3]),
      close: this.safeStod(k[4]),
      volume: this.safeStod(k[5]),
    }));
  }

  async placeOrder(order: OrderRequest): Promise<OrderResponse> {
    const params: any = {
      symbol: order.symbol,
      side: order.side,
      type: order.type,
      quantity: order.quantity,
    };

    if (order.price) {
      params.price = order.price;
      params.timeInForce = order.timeInForce || 'GTC';
    }

    if (order.stopPrice) {
      params.stopPrice = order.stopPrice;
    }

    if (order.reduceOnly) {
      params.reduceOnly = 'true';
    }

    const data = await this.httpPost('/fapi/v1/order', params, true);

    return {
      orderId: data.orderId.toString(),
      status: data.status,
      symbol: data.symbol,
      side: data.side,
      type: data.type,
      quantity: this.safeStod(data.origQty),
      filledQuantity: this.safeStod(data.executedQty),
      price: this.safeStod(data.price),
      averagePrice: this.safeStod(data.avgPrice),
    };
  }

  async cancelOrder(symbol: string, orderId: string): Promise<void> {
    await this.httpDelete('/fapi/v1/order', {
      symbol,
      orderId,
    }, true);
  }

  async getBalance(): Promise<{ [asset: string]: number }> {
    const data = await this.httpGet('/fapi/v2/account', {}, true);

    if (!data || !Array.isArray(data.assets)) {
      throw new Error('Invalid account data');
    }

    const balances: { [asset: string]: number } = {};

    for (const asset of data.assets) {
      if (asset && asset.asset) {
        balances[asset.asset] = this.safeStod(asset.walletBalance);
      }
    }

    return balances;
  }

  async getPositions(): Promise<Position[]> {
    const data = await this.httpGet('/fapi/v2/positionRisk', {}, true);

    if (!Array.isArray(data)) {
      throw new Error('Invalid position data');
    }

    const positions: Position[] = [];

    for (const pos of data) {
      const qty = this.safeStod(pos.positionAmt);

      if (Math.abs(qty) < 1e-8) {
        continue; // Skip zero positions
      }

      positions.push({
        symbol: pos.symbol,
        side: qty > 0 ? 'LONG' : 'SHORT',
        quantity: Math.abs(qty),
        entryPrice: this.safeStod(pos.entryPrice),
        unrealizedPnl: this.safeStod(pos.unRealizedProfit),
        leverage: this.safeStod(pos.leverage, 1),
        liquidationPrice: this.safeStod(pos.liquidationPrice),
      });
    }

    return positions;
  }

  async setLeverage(symbol: string, leverage: number): Promise<void> {
    await this.httpPost('/fapi/v1/leverage', {
      symbol,
      leverage,
    }, true);
  }

  getWebSocketUrl(symbol: string, streams: string[]): string {
    const streamStr = streams.join('/');
    return `${this.wsUrl}/stream?streams=${streamStr}`;
  }
}
