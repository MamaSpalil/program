import { Router } from 'express';
import { PrismaClient } from '@prisma/client';
import { AuthRequest } from '../middleware/auth';
import { ApiError } from '../middleware/errorHandler';

const router = Router();
const prisma = new PrismaClient();

// GET /api/v1/users/me
router.get('/me', async (req: AuthRequest, res, next) => {
  try {
    const user = await prisma.user.findUnique({
      where: { id: req.user!.id },
      select: {
        id: true,
        email: true,
        firstName: true,
        lastName: true,
        role: true,
        status: true,
        emailVerified: true,
        twoFactorEnabled: true,
        createdAt: true,
        lastLoginAt: true,
        settings: true,
      },
    });

    if (!user) {
      throw new ApiError(404, 'User not found');
    }

    res.json({
      success: true,
      data: user,
    });
  } catch (error) {
    next(error);
  }
});

// PUT /api/v1/users/me
router.put('/me', async (req: AuthRequest, res, next) => {
  try {
    const { firstName, lastName } = req.body;

    const user = await prisma.user.update({
      where: { id: req.user!.id },
      data: {
        firstName,
        lastName,
      },
      select: {
        id: true,
        email: true,
        firstName: true,
        lastName: true,
        role: true,
      },
    });

    res.json({
      success: true,
      data: user,
    });
  } catch (error) {
    next(error);
  }
});

// GET /api/v1/users/me/settings
router.get('/me/settings', async (req: AuthRequest, res, next) => {
  try {
    const settings = await prisma.userSettings.findUnique({
      where: { userId: req.user!.id },
    });

    if (!settings) {
      throw new ApiError(404, 'Settings not found');
    }

    res.json({
      success: true,
      data: settings,
    });
  } catch (error) {
    next(error);
  }
});

// PUT /api/v1/users/me/settings
router.put('/me/settings', async (req: AuthRequest, res, next) => {
  try {
    const settings = await prisma.userSettings.upsert({
      where: { userId: req.user!.id },
      update: req.body,
      create: {
        userId: req.user!.id,
        ...req.body,
      },
    });

    res.json({
      success: true,
      data: settings,
    });
  } catch (error) {
    next(error);
  }
});

export default router;
