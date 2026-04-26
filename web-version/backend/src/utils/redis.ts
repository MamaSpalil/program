import Redis from 'ioredis';
import config from '../config';
import logger from './logger';

class RedisClientClass {
  private client: Redis | null = null;
  private subscriber: Redis | null = null;
  private publisher: Redis | null = null;

  async connect(): Promise<void> {
    if (this.client) {
      logger.warn('Redis client already connected');
      return;
    }

    const redisOptions = {
      host: config.redisHost,
      port: config.redisPort,
      password: config.redisPassword || undefined,
      retryStrategy: (times: number) => {
        const delay = Math.min(times * 50, 2000);
        return delay;
      },
      maxRetriesPerRequest: 3,
    };

    this.client = new Redis(redisOptions);
    this.subscriber = new Redis(redisOptions);
    this.publisher = new Redis(redisOptions);

    this.client.on('connect', () => {
      logger.info('Redis client connected');
    });

    this.client.on('error', (err) => {
      logger.error('Redis client error:', err);
    });

    this.subscriber.on('connect', () => {
      logger.info('Redis subscriber connected');
    });

    this.publisher.on('connect', () => {
      logger.info('Redis publisher connected');
    });
  }

  async disconnect(): Promise<void> {
    if (this.client) {
      await this.client.quit();
      this.client = null;
    }
    if (this.subscriber) {
      await this.subscriber.quit();
      this.subscriber = null;
    }
    if (this.publisher) {
      await this.publisher.quit();
      this.publisher = null;
    }
    logger.info('Redis disconnected');
  }

  getClient(): Redis {
    if (!this.client) {
      throw new Error('Redis client not connected');
    }
    return this.client;
  }

  getSubscriber(): Redis {
    if (!this.subscriber) {
      throw new Error('Redis subscriber not connected');
    }
    return this.subscriber;
  }

  getPublisher(): Redis {
    if (!this.publisher) {
      throw new Error('Redis publisher not connected');
    }
    return this.publisher;
  }

  // Cache helpers
  async get(key: string): Promise<string | null> {
    return this.getClient().get(key);
  }

  async set(key: string, value: string, ttl?: number): Promise<void> {
    if (ttl) {
      await this.getClient().setex(key, ttl, value);
    } else {
      await this.getClient().set(key, value);
    }
  }

  async del(key: string): Promise<void> {
    await this.getClient().del(key);
  }

  async exists(key: string): Promise<boolean> {
    const result = await this.getClient().exists(key);
    return result === 1;
  }

  // Pub/Sub helpers
  async publish(channel: string, message: string): Promise<void> {
    await this.getPublisher().publish(channel, message);
  }

  async subscribe(channel: string, callback: (message: string) => void): Promise<void> {
    const subscriber = this.getSubscriber();
    await subscriber.subscribe(channel);
    subscriber.on('message', (ch, msg) => {
      if (ch === channel) {
        callback(msg);
      }
    });
  }

  async unsubscribe(channel: string): Promise<void> {
    await this.getSubscriber().unsubscribe(channel);
  }
}

export const RedisClient = new RedisClientClass();
