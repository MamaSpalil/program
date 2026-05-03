import { Router } from 'express';
import { PrismaClient } from '@prisma/client';
import { z } from 'zod';
import { AuthRequest } from '../middleware/auth';
import { ApiError } from '../middleware/errorHandler';
import { Encryption } from '../utils/encryption';

const router = Router();
const prisma = new PrismaClient();

// Validation schema
const addExchangeSchema = z.object({
  exchangeName: z.enum(['binance', 'bybit', 'okx', 'bitget', 'kucoin']),
  apiKey: z.string(),
  apiSecret: z.string(),
  passphrase: z.string().optional(),
  testnet: z.boolean().default(true),
  label: z.string().optional(),
});

// GET /api/v1/exchanges
router.get('/', async (req: AuthRequest, res, next) => {
  try {
    const exchanges = await prisma.userExchange.findMany({
      where: { userId: req.user!.id },
      select: {
        id: true,
        exchangeName: true,
        testnet: true,
        enabled: true,
        label: true,
        createdAt: true,
        // Don't return API keys
      },
    });

    res.json({
      success: true,
      data: exchanges,
    });
  } catch (error) {
    next(error);
  }
});

// POST /api/v1/exchanges
router.post('/', async (req: AuthRequest, res, next) => {
  try {
    const validated = addExchangeSchema.parse(req.body);

    // Encrypt API credentials
    const apiKeyEncrypted = Encryption.encrypt(validated.apiKey);
    const apiSecretEncrypted = Encryption.encrypt(validated.apiSecret);
    const passphraseEncrypted = validated.passphrase
      ? Encryption.encrypt(validated.passphrase)
      : null;

    const exchange = await prisma.userExchange.create({
      data: {
        userId: req.user!.id,
        exchangeName: validated.exchangeName,
        apiKeyEncrypted,
        apiSecretEncrypted,
        passphraseEncrypted,
        testnet: validated.testnet,
        label: validated.label,
      },
      select: {
        id: true,
        exchangeName: true,
        testnet: true,
        enabled: true,
        label: true,
        createdAt: true,
      },
    });

    res.status(201).json({
      success: true,
      data: exchange,
    });
  } catch (error) {
    if (error instanceof z.ZodError) {
      next(new ApiError(400, 'Validation error: ' + error.errors[0].message));
    } else {
      next(error);
    }
  }
});

// PUT /api/v1/exchanges/:id
router.put('/:id', async (req: AuthRequest, res, next) => {
  try {
    const { id } = req.params;
    const { enabled, label } = req.body;

    // Verify ownership
    const existing = await prisma.userExchange.findFirst({
      where: { id, userId: req.user!.id },
    });

    if (!existing) {
      throw new ApiError(404, 'Exchange configuration not found');
    }

    const exchange = await prisma.userExchange.update({
      where: { id },
      data: { enabled, label },
      select: {
        id: true,
        exchangeName: true,
        testnet: true,
        enabled: true,
        label: true,
      },
    });

    res.json({
      success: true,
      data: exchange,
    });
  } catch (error) {
    next(error);
  }
});

// DELETE /api/v1/exchanges/:id
router.delete('/:id', async (req: AuthRequest, res, next) => {
  try {
    const { id } = req.params;

    // Verify ownership
    const existing = await prisma.userExchange.findFirst({
      where: { id, userId: req.user!.id },
    });

    if (!existing) {
      throw new ApiError(404, 'Exchange configuration not found');
    }

    await prisma.userExchange.delete({
      where: { id },
    });

    res.json({
      success: true,
      message: 'Exchange configuration deleted',
    });
  } catch (error) {
    next(error);
  }
});

export default router;
