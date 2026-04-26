import crypto from 'crypto';
import axios, { AxiosInstance, AxiosRequestConfig } from 'axios';
import logger from '../utils/logger';

export interface ExchangeConfig {
  apiKey: string;
  apiSecret: string;
  passphrase?: string;
  testnet: boolean;
}

export interface Candle {
  timestamp: number;
  open: number;
  high: number;
  low: number;
  close: number;
  volume: number;
}

export interface OrderRequest {
  symbol: string;
  side: 'BUY' | 'SELL';
  type: 'MARKET' | 'LIMIT' | 'STOP_MARKET' | 'STOP_LIMIT';
  quantity: number;
  price?: number;
  stopPrice?: number;
  reduceOnly?: boolean;
  timeInForce?: string;
}

export interface OrderResponse {
  orderId: string;
  status: string;
  symbol: string;
  side: string;
  type: string;
  quantity: number;
  filledQuantity: number;
  price?: number;
  averagePrice?: number;
}

export interface Position {
  symbol: string;
  side: 'LONG' | 'SHORT';
  quantity: number;
  entryPrice: number;
  unrealizedPnl: number;
  leverage: number;
  liquidationPrice?: number;
}

export abstract class BaseExchange {
  protected config: ExchangeConfig;
  protected httpClient: AxiosInstance;
  protected name: string;
  protected baseUrl: string;
  protected wsUrl: string;

  constructor(name: string, config: ExchangeConfig) {
    this.name = name;
    this.config = config;
    this.baseUrl = '';
    this.wsUrl = '';
    this.httpClient = axios.create({
      timeout: 10000,
      headers: {
        'Content-Type': 'application/json',
      },
    });
  }

  // Abstract methods that must be implemented by each exchange
  abstract getPrice(symbol: string): Promise<number>;
  abstract getKlines(symbol: string, interval: string, limit?: number): Promise<Candle[]>;
  abstract placeOrder(order: OrderRequest): Promise<OrderResponse>;
  abstract cancelOrder(symbol: string, orderId: string): Promise<void>;
  abstract getBalance(): Promise<{ [asset: string]: number }>;
  abstract getPositions(): Promise<Position[]>;
  abstract setLeverage(symbol: string, leverage: number): Promise<void>;

  // HTTP request helpers
  protected async httpGet(
    endpoint: string,
    params: any = {},
    signed: boolean = false
  ): Promise<any> {
    const url = `${this.baseUrl}${endpoint}`;
    const config: AxiosRequestConfig = { params };

    if (signed) {
      this.signRequest(config, 'GET', endpoint, params);
    }

    try {
      const response = await this.httpClient.get(url, config);
      return response.data;
    } catch (error: any) {
      logger.error(`${this.name} HTTP GET error:`, error.response?.data || error.message);
      throw new Error(`${this.name} API error: ${error.response?.data?.msg || error.message}`);
    }
  }

  protected async httpPost(
    endpoint: string,
    data: any = {},
    signed: boolean = false
  ): Promise<any> {
    const url = `${this.baseUrl}${endpoint}`;
    const config: AxiosRequestConfig = {};

    if (signed) {
      this.signRequest(config, 'POST', endpoint, data);
    }

    try {
      const response = await this.httpClient.post(url, data, config);
      return response.data;
    } catch (error: any) {
      logger.error(`${this.name} HTTP POST error:`, error.response?.data || error.message);
      throw new Error(`${this.name} API error: ${error.response?.data?.msg || error.message}`);
    }
  }

  protected async httpDelete(
    endpoint: string,
    params: any = {},
    signed: boolean = false
  ): Promise<any> {
    const url = `${this.baseUrl}${endpoint}`;
    const config: AxiosRequestConfig = { params };

    if (signed) {
      this.signRequest(config, 'DELETE', endpoint, params);
    }

    try {
      const response = await this.httpClient.delete(url, config);
      return response.data;
    } catch (error: any) {
      logger.error(`${this.name} HTTP DELETE error:`, error.response?.data || error.message);
      throw new Error(`${this.name} API error: ${error.response?.data?.msg || error.message}`);
    }
  }

  // Sign request - to be implemented by each exchange
  protected abstract signRequest(
    config: AxiosRequestConfig,
    method: string,
    endpoint: string,
    params: any
  ): void;

  // Helper to create HMAC signature
  protected createSignature(secret: string, data: string): string {
    return crypto.createHmac('sha256', secret).update(data).digest('hex');
  }

  // Safe string to double conversion
  protected safeStod(value: any, defaultValue: number = 0): number {
    if (value === null || value === undefined || value === '') {
      return defaultValue;
    }
    const num = typeof value === 'string' ? parseFloat(value) : Number(value);
    return isNaN(num) ? defaultValue : num;
  }
}
