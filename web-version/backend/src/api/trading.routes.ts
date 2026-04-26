import { Router } from 'express';
import { PrismaClient } from '@prisma/client';
import { AuthRequest } from '../middleware/auth';
import { ApiError } from '../middleware/errorHandler';

const router = Router();
const prisma = new PrismaClient();

// GET /api/v1/trading/orders
router.get('/orders', async (req: AuthRequest, res, next) => {
  try {
    const { status, exchange, symbol, limit = '50' } = req.query;

    const where: any = {
      userId: req.user!.id,
    };

    if (status) where.status = status;
    if (exchange) where.exchange = exchange;
    if (symbol) where.symbol = symbol;

    const orders = await prisma.order.findMany({
      where,
      orderBy: { createdAt: 'desc' },
      take: parseInt(limit as string),
    });

    res.json({
      success: true,
      data: orders,
    });
  } catch (error) {
    next(error);
  }
});

// GET /api/v1/trading/positions
router.get('/positions', async (req: AuthRequest, res, next) => {
  try {
    const { exchange, symbol } = req.query;

    const where: any = {
      userId: req.user!.id,
      status: 'OPEN',
    };

    if (exchange) where.exchange = exchange;
    if (symbol) where.symbol = symbol;

    const positions = await prisma.position.findMany({
      where,
      orderBy: { createdAt: 'desc' },
    });

    res.json({
      success: true,
      data: positions,
    });
  } catch (error) {
    next(error);
  }
});

// GET /api/v1/trading/trades
router.get('/trades', async (req: AuthRequest, res, next) => {
  try {
    const { exchange, symbol, limit = '100' } = req.query;

    const where: any = {
      userId: req.user!.id,
    };

    if (exchange) where.exchange = exchange;
    if (symbol) where.symbol = symbol;

    const trades = await prisma.trade.findMany({
      where,
      orderBy: { timestamp: 'desc' },
      take: parseInt(limit as string),
    });

    res.json({
      success: true,
      data: trades,
    });
  } catch (error) {
    next(error);
  }
});

// GET /api/v1/trading/equity
router.get('/equity', async (req: AuthRequest, res, next) => {
  try {
    const { period = '24h' } = req.query;

    // Calculate time range
    const now = new Date();
    const startTime = new Date();

    switch (period) {
      case '1h':
        startTime.setHours(now.getHours() - 1);
        break;
      case '24h':
        startTime.setHours(now.getHours() - 24);
        break;
      case '7d':
        startTime.setDate(now.getDate() - 7);
        break;
      case '30d':
        startTime.setDate(now.getDate() - 30);
        break;
      default:
        startTime.setHours(now.getHours() - 24);
    }

    const equityHistory = await prisma.equityHistory.findMany({
      where: {
        userId: req.user!.id,
        timestamp: {
          gte: startTime,
        },
      },
      orderBy: { timestamp: 'asc' },
    });

    res.json({
      success: true,
      data: equityHistory,
    });
  } catch (error) {
    next(error);
  }
});

export default router;
