import express, { Application } from 'express';
import cors from 'cors';
import helmet from 'helmet';
import compression from 'compression';
import morgan from 'morgan';
import { createServer } from 'http';
import { Server as SocketIOServer } from 'socket.io';
import config from './config';
import logger from './utils/logger';
import { errorHandler } from './middleware/errorHandler';
import { rateLimiter } from './middleware/rateLimiter';
import routes from './api/routes';
import { WebSocketManager } from './websocket/WebSocketManager';
import { RedisClient } from './utils/redis';

class Server {
  private app: Application;
  private httpServer: ReturnType<typeof createServer>;
  private io: SocketIOServer;
  private wsManager: WebSocketManager;

  constructor() {
    this.app = express();
    this.httpServer = createServer(this.app);
    this.io = new SocketIOServer(this.httpServer, {
      cors: {
        origin: config.corsOrigin,
        credentials: true,
      },
    });
    this.wsManager = new WebSocketManager(this.io);

    this.setupMiddleware();
    this.setupRoutes();
    this.setupErrorHandling();
  }

  private setupMiddleware(): void {
    // Security
    this.app.use(helmet());
    this.app.use(cors({ origin: config.corsOrigin, credentials: true }));

    // Parsing
    this.app.use(express.json({ limit: '10mb' }));
    this.app.use(express.urlencoded({ extended: true, limit: '10mb' }));

    // Compression
    this.app.use(compression());

    // Logging
    if (config.nodeEnv !== 'test') {
      this.app.use(
        morgan('combined', {
          stream: { write: (message) => logger.info(message.trim()) },
        })
      );
    }

    // Rate limiting
    this.app.use(rateLimiter);
  }

  private setupRoutes(): void {
    // Health check
    this.app.get('/health', (req, res) => {
      res.json({
        status: 'ok',
        timestamp: new Date().toISOString(),
        uptime: process.uptime(),
      });
    });

    // API routes
    this.app.use(config.apiPrefix, routes);

    // 404 handler
    this.app.use((req, res) => {
      res.status(404).json({
        success: false,
        error: 'Route not found',
      });
    });
  }

  private setupErrorHandling(): void {
    this.app.use(errorHandler);

    // Handle uncaught exceptions
    process.on('uncaughtException', (error: Error) => {
      logger.error('Uncaught Exception:', error);
      this.shutdown(1);
    });

    // Handle unhandled promise rejections
    process.on('unhandledRejection', (reason: any) => {
      logger.error('Unhandled Rejection:', reason);
      this.shutdown(1);
    });

    // Handle SIGTERM
    process.on('SIGTERM', () => {
      logger.info('SIGTERM received, shutting down gracefully');
      this.shutdown(0);
    });

    // Handle SIGINT
    process.on('SIGINT', () => {
      logger.info('SIGINT received, shutting down gracefully');
      this.shutdown(0);
    });
  }

  public async start(): Promise<void> {
    try {
      // Connect to Redis
      await RedisClient.connect();
      logger.info('Connected to Redis');

      // Start WebSocket manager
      await this.wsManager.start();
      logger.info('WebSocket manager started');

      // Start HTTP server
      this.httpServer.listen(config.port, () => {
        logger.info(`Server running on port ${config.port} in ${config.nodeEnv} mode`);
        logger.info(`API available at http://localhost:${config.port}${config.apiPrefix}`);
      });
    } catch (error) {
      logger.error('Failed to start server:', error);
      process.exit(1);
    }
  }

  private async shutdown(code: number): Promise<void> {
    try {
      logger.info('Shutting down server...');

      // Stop accepting new connections
      this.httpServer.close(() => {
        logger.info('HTTP server closed');
      });

      // Stop WebSocket manager
      await this.wsManager.stop();

      // Disconnect Redis
      await RedisClient.disconnect();
      logger.info('Disconnected from Redis');

      logger.info('Shutdown complete');
      process.exit(code);
    } catch (error) {
      logger.error('Error during shutdown:', error);
      process.exit(1);
    }
  }
}

// Start server
const server = new Server();
server.start();

export default server;
