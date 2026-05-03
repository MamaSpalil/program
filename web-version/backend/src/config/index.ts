import dotenv from 'dotenv';
dotenv.config();

interface Config {
  nodeEnv: string;
  port: number;
  apiPrefix: string;
  databaseUrl: string;
  redisHost: string;
  redisPort: number;
  redisPassword: string;
  jwtSecret: string;
  jwtExpiresIn: string;
  jwtRefreshExpiresIn: string;
  encryptionKey: string;
  mlServiceUrl: string;
  rateLimitWindowMs: number;
  rateLimitMaxRequests: number;
  corsOrigin: string;
  logLevel: string;
}

const config: Config = {
  nodeEnv: process.env.NODE_ENV || 'development',
  port: parseInt(process.env.PORT || '4000', 10),
  apiPrefix: process.env.API_PREFIX || '/api/v1',
  databaseUrl: process.env.DATABASE_URL || '',
  redisHost: process.env.REDIS_HOST || 'localhost',
  redisPort: parseInt(process.env.REDIS_PORT || '6379', 10),
  redisPassword: process.env.REDIS_PASSWORD || '',
  jwtSecret: process.env.JWT_SECRET || 'change-this-secret',
  jwtExpiresIn: process.env.JWT_EXPIRES_IN || '7d',
  jwtRefreshExpiresIn: process.env.JWT_REFRESH_EXPIRES_IN || '30d',
  encryptionKey: process.env.ENCRYPTION_KEY || 'change-this-32-char-encryption-key',
  mlServiceUrl: process.env.ML_SERVICE_URL || 'http://localhost:8000',
  rateLimitWindowMs: parseInt(process.env.RATE_LIMIT_WINDOW_MS || '60000', 10),
  rateLimitMaxRequests: parseInt(process.env.RATE_LIMIT_MAX_REQUESTS || '100', 10),
  corsOrigin: process.env.CORS_ORIGIN || 'http://localhost:3000',
  logLevel: process.env.LOG_LEVEL || 'info',
};

// Validate required environment variables
const requiredEnvVars = ['DATABASE_URL', 'JWT_SECRET', 'ENCRYPTION_KEY'];
const missingEnvVars = requiredEnvVars.filter((envVar) => !process.env[envVar]);

if (missingEnvVars.length > 0) {
  throw new Error(
    `Missing required environment variables: ${missingEnvVars.join(', ')}`
  );
}

// Validate encryption key length (must be 32 characters for AES-256)
if (config.encryptionKey.length !== 32) {
  throw new Error('ENCRYPTION_KEY must be exactly 32 characters long');
}

export default config;
