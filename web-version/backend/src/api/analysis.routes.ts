import { Router } from 'express';
import { PrismaClient } from '@prisma/client';
import { AuthRequest } from '../middleware/auth';
import { ApiError } from '../middleware/errorHandler';
import { ChartAnalysis } from '../analysis/ChartAnalysis';
import { SignalGenerator } from '../analysis/SignalGenerator';

const router = Router();
const prisma = new PrismaClient();

// GET /api/v1/analysis/zones/:exchange/:symbol/:interval
router.get('/zones/:exchange/:symbol/:interval', async (req: AuthRequest, res, next) => {
  try {
    const { exchange, symbol, interval } = req.params;
    const { limit = '200' } = req.query;

    // Fetch candles
    const candles = await prisma.candle.findMany({
      where: { exchange, symbol, interval },
      orderBy: { timestamp: 'asc' },
      take: parseInt(limit as string),
      select: {
        timestamp: true,
        open: true,
        high: true,
        low: true,
        close: true,
        volume: true,
      },
    });

    if (candles.length < 20) {
      throw new ApiError(400, 'Insufficient historical data');
    }

    // Convert to analysis format
    const candleData = candles.map(c => ({
      timestamp: c.timestamp.getTime(),
      open: c.open,
      high: c.high,
      low: c.low,
      close: c.close,
      volume: c.volume,
    }));

    // Detect support/resistance zones
    const zones = ChartAnalysis.detectSupportResistanceZones(candleData);

    // Calculate pivot points
    const pivots = ChartAnalysis.calculatePivotPoints(candleData);

    // Calculate Fibonacci levels
    const fibonacci = ChartAnalysis.calculateFibonacciLevels(candleData);

    // Detect confluences
    const confluences = ChartAnalysis.detectZoneConfluence(zones);

    // Analyze volume profile
    const volumeProfile = ChartAnalysis.analyzeVolumeProfile(candleData);

    res.json({
      success: true,
      data: {
        zones,
        pivots,
        fibonacci,
        confluences,
        volumeProfile,
      },
    });
  } catch (error) {
    next(error);
  }
});

// GET /api/v1/analysis/signals/:exchange/:symbol/:interval
router.get('/signals/:exchange/:symbol/:interval', async (req: AuthRequest, res, next) => {
  try {
    const { exchange, symbol, interval } = req.params;
    const { limit = '200' } = req.query;

    // Fetch candles
    const candles = await prisma.candle.findMany({
      where: { exchange, symbol, interval },
      orderBy: { timestamp: 'asc' },
      take: parseInt(limit as string),
      select: {
        timestamp: true,
        open: true,
        high: true,
        low: true,
        close: true,
        volume: true,
      },
    });

    if (candles.length < 20) {
      throw new ApiError(400, 'Insufficient historical data');
    }

    // Convert to analysis format
    const candleData = candles.map(c => ({
      timestamp: c.timestamp.getTime(),
      open: c.open,
      high: c.high,
      low: c.low,
      close: c.close,
      volume: c.volume,
    }));

    // Detect zones
    const zones = ChartAnalysis.detectSupportResistanceZones(candleData);

    // Generate signals
    const signals = SignalGenerator.generateSignals(candleData, zones);

    // Get active positions for exit signals
    const activePositions = await prisma.position.findMany({
      where: {
        userId: req.user!.id,
        exchange,
        symbol,
        status: 'OPEN',
      },
      select: {
        side: true,
        entryPrice: true,
        stopLoss: true,
        takeProfit: true,
      },
    });

    const positionsForAnalysis = activePositions.map(p => ({
      type: p.side,
      entryPrice: p.entryPrice,
      stopLoss: p.stopLoss || 0,
      takeProfit: p.takeProfit || 0,
    }));

    // Generate exit signals
    const exitSignals = SignalGenerator.generateExitSignals(candleData, zones, positionsForAnalysis);

    res.json({
      success: true,
      data: {
        entrySignals: signals,
        exitSignals,
        totalSignals: signals.length + exitSignals.length,
      },
    });
  } catch (error) {
    next(error);
  }
});

// POST /api/v1/analysis/markers
router.post('/markers', async (req: AuthRequest, res, next) => {
  try {
    const { exchange, symbol, interval, type, price, reason, confidence, stopLoss, takeProfit, zoneId } = req.body;

    const marker = await prisma.positionMarker.create({
      data: {
        userId: req.user!.id,
        exchange,
        symbol,
        interval,
        type,
        price,
        timestamp: new Date(),
        reason,
        confidence,
        stopLoss,
        takeProfit,
        zoneId,
      },
    });

    res.status(201).json({
      success: true,
      data: marker,
    });
  } catch (error) {
    next(error);
  }
});

// GET /api/v1/analysis/markers/:exchange/:symbol
router.get('/markers/:exchange/:symbol', async (req: AuthRequest, res, next) => {
  try {
    const { exchange, symbol } = req.params;
    const { interval, type, executed } = req.query;

    const where: any = {
      userId: req.user!.id,
      exchange,
      symbol,
    };

    if (interval) where.interval = interval;
    if (type) where.type = type;
    if (executed !== undefined) where.isExecuted = executed === 'true';

    const markers = await prisma.positionMarker.findMany({
      where,
      orderBy: { timestamp: 'desc' },
      take: 100,
    });

    res.json({
      success: true,
      data: markers,
    });
  } catch (error) {
    next(error);
  }
});

// PUT /api/v1/analysis/markers/:id/execute
router.put('/markers/:id/execute', async (req: AuthRequest, res, next) => {
  try {
    const { id } = req.params;
    const { pnl } = req.body;

    // Verify ownership
    const marker = await prisma.positionMarker.findFirst({
      where: { id, userId: req.user!.id },
    });

    if (!marker) {
      throw new ApiError(404, 'Marker not found');
    }

    const updatedMarker = await prisma.positionMarker.update({
      where: { id },
      data: {
        isExecuted: true,
        executedAt: new Date(),
        pnl: pnl || null,
      },
    });

    res.json({
      success: true,
      data: updatedMarker,
    });
  } catch (error) {
    next(error);
  }
});

export default router;
