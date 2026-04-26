import { Server as SocketIOServer, Socket } from 'socket.io';
import { verify } from 'jsonwebtoken';
import config from '../config';
import logger from '../utils/logger';
import { RedisClient } from '../utils/redis';

interface AuthenticatedSocket extends Socket {
  userId?: string;
}

export class WebSocketManager {
  private io: SocketIOServer;
  private userSockets: Map<string, Set<string>> = new Map(); // userId -> Set of socketIds

  constructor(io: SocketIOServer) {
    this.io = io;
    this.setupMiddleware();
    this.setupEventHandlers();
  }

  private setupMiddleware(): void {
    // Authentication middleware
    this.io.use(async (socket: AuthenticatedSocket, next) => {
      try {
        const token = socket.handshake.auth.token || socket.handshake.headers.authorization;

        if (!token) {
          return next(new Error('Authentication required'));
        }

        const tokenStr = token.startsWith('Bearer ') ? token.substring(7) : token;

        const decoded = verify(tokenStr, config.jwtSecret) as {
          userId: string;
          type: string;
        };

        if (decoded.type !== 'access') {
          return next(new Error('Invalid token type'));
        }

        socket.userId = decoded.userId;
        next();
      } catch (error) {
        logger.error('WebSocket authentication error:', error);
        next(new Error('Authentication failed'));
      }
    });
  }

  private setupEventHandlers(): void {
    this.io.on('connection', (socket: AuthenticatedSocket) => {
      const userId = socket.userId!;
      logger.info(`WebSocket connected: userId=${userId}, socketId=${socket.id}`);

      // Track user sockets
      if (!this.userSockets.has(userId)) {
        this.userSockets.set(userId, new Set());
      }
      this.userSockets.get(userId)!.add(socket.id);

      // Join user-specific room
      socket.join(`user:${userId}`);

      // Handle subscriptions
      socket.on('subscribe', (data: { channel: string; params?: any }) => {
        this.handleSubscribe(socket, data);
      });

      socket.on('unsubscribe', (data: { channel: string }) => {
        this.handleUnsubscribe(socket, data);
      });

      // Handle disconnect
      socket.on('disconnect', () => {
        logger.info(`WebSocket disconnected: userId=${userId}, socketId=${socket.id}`);

        const sockets = this.userSockets.get(userId);
        if (sockets) {
          sockets.delete(socket.id);
          if (sockets.size === 0) {
            this.userSockets.delete(userId);
          }
        }
      });
    });
  }

  private handleSubscribe(socket: AuthenticatedSocket, data: { channel: string; params?: any }): void {
    const { channel, params } = data;
    const userId = socket.userId!;

    logger.debug(`Subscribe request: userId=${userId}, channel=${channel}`, params);

    // Join channel room
    const roomName = params
      ? `${channel}:${JSON.stringify(params)}`
      : channel;

    socket.join(roomName);

    socket.emit('subscribed', { channel, params, roomName });
  }

  private handleUnsubscribe(socket: AuthenticatedSocket, data: { channel: string }): void {
    const { channel } = data;
    const userId = socket.userId!;

    logger.debug(`Unsubscribe request: userId=${userId}, channel=${channel}`);

    // Leave all rooms matching this channel
    const rooms = Array.from(socket.rooms);
    for (const room of rooms) {
      if (room.startsWith(channel)) {
        socket.leave(room);
      }
    }

    socket.emit('unsubscribed', { channel });
  }

  // Broadcast to specific user
  public broadcastToUser(userId: string, event: string, data: any): void {
    this.io.to(`user:${userId}`).emit(event, data);
  }

  // Broadcast to channel
  public broadcastToChannel(channel: string, event: string, data: any): void {
    this.io.to(channel).emit(event, data);
  }

  // Broadcast to all connected clients
  public broadcast(event: string, data: any): void {
    this.io.emit(event, data);
  }

  public async start(): Promise<void> {
    // Subscribe to Redis pub/sub channels for distributed broadcasting
    await RedisClient.subscribe('market:update', (message) => {
      const data = JSON.parse(message);
      this.broadcastToChannel(`market:${data.symbol}`, 'market:update', data);
    });

    await RedisClient.subscribe('trade:update', (message) => {
      const data = JSON.parse(message);
      this.broadcastToUser(data.userId, 'trade:update', data);
    });

    await RedisClient.subscribe('order:update', (message) => {
      const data = JSON.parse(message);
      this.broadcastToUser(data.userId, 'order:update', data);
    });

    await RedisClient.subscribe('position:update', (message) => {
      const data = JSON.parse(message);
      this.broadcastToUser(data.userId, 'position:update', data);
    });

    logger.info('WebSocket manager started and subscribed to Redis channels');
  }

  public async stop(): Promise<void> {
    // Unsubscribe from Redis channels
    await RedisClient.unsubscribe('market:update');
    await RedisClient.unsubscribe('trade:update');
    await RedisClient.unsubscribe('order:update');
    await RedisClient.unsubscribe('position:update');

    // Disconnect all sockets
    this.io.disconnectSockets();

    logger.info('WebSocket manager stopped');
  }

  public getConnectedUsers(): number {
    return this.userSockets.size;
  }

  public getConnectedSockets(): number {
    return this.io.sockets.sockets.size;
  }
}
