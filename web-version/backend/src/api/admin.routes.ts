import { Router } from 'express';
import { z } from 'zod';
import { PrismaClient } from '@prisma/client';
import { AuthRequest, authorize } from '../middleware/auth';
import { ApiError } from '../middleware/errorHandler';
import InvitationService from '../auth/InvitationService';

const router = Router();
const prisma = new PrismaClient();

// All admin routes require ADMIN role
router.use(authorize('ADMIN'));

// Validation schemas
const createInvitationSchema = z.object({
  email: z.string().email(),
  expiresInDays: z.number().min(1).max(30).default(7),
});

// POST /api/v1/admin/invitations
router.post('/invitations', async (req: AuthRequest, res, next) => {
  try {
    const validated = createInvitationSchema.parse(req.body);
    const invitation = await InvitationService.generateInvitation(
      req.user!.id,
      validated.email,
      validated.expiresInDays
    );

    res.status(201).json({
      success: true,
      data: invitation,
    });
  } catch (error) {
    if (error instanceof z.ZodError) {
      next(new ApiError(400, 'Validation error: ' + error.errors[0].message));
    } else {
      next(error);
    }
  }
});

// GET /api/v1/admin/invitations
router.get('/invitations', async (req: AuthRequest, res, next) => {
  try {
    const invitations = await InvitationService.getInvitationsByAdmin(req.user!.id);

    res.json({
      success: true,
      data: invitations,
    });
  } catch (error) {
    next(error);
  }
});

// DELETE /api/v1/admin/invitations/:id
router.delete('/invitations/:id', async (req: AuthRequest, res, next) => {
  try {
    const { id } = req.params;
    await InvitationService.revokeInvitation(req.user!.id, id);

    res.json({
      success: true,
      message: 'Invitation revoked successfully',
    });
  } catch (error) {
    next(error);
  }
});

// GET /api/v1/admin/users
router.get('/users', async (req: AuthRequest, res, next) => {
  try {
    const { status, role, limit = '50', offset = '0' } = req.query;

    const where: any = {};
    if (status) where.status = status;
    if (role) where.role = role;

    const users = await prisma.user.findMany({
      where,
      select: {
        id: true,
        email: true,
        firstName: true,
        lastName: true,
        role: true,
        status: true,
        emailVerified: true,
        createdAt: true,
        lastLoginAt: true,
      },
      orderBy: { createdAt: 'desc' },
      take: parseInt(limit as string),
      skip: parseInt(offset as string),
    });

    const total = await prisma.user.count({ where });

    res.json({
      success: true,
      data: {
        users,
        total,
        limit: parseInt(limit as string),
        offset: parseInt(offset as string),
      },
    });
  } catch (error) {
    next(error);
  }
});

// PUT /api/v1/admin/users/:id/role
router.put('/users/:id/role', async (req: AuthRequest, res, next) => {
  try {
    const { id } = req.params;
    const { role } = req.body;

    if (!['USER', 'ADMIN', 'DEVELOPER'].includes(role)) {
      throw new ApiError(400, 'Invalid role');
    }

    // Prevent changing own role
    if (id === req.user!.id) {
      throw new ApiError(400, 'Cannot change your own role');
    }

    const user = await prisma.user.update({
      where: { id },
      data: { role },
      select: {
        id: true,
        email: true,
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

// PUT /api/v1/admin/users/:id/status
router.put('/users/:id/status', async (req: AuthRequest, res, next) => {
  try {
    const { id } = req.params;
    const { status } = req.body;

    if (!['ACTIVE', 'SUSPENDED', 'DELETED'].includes(status)) {
      throw new ApiError(400, 'Invalid status');
    }

    // Prevent changing own status
    if (id === req.user!.id) {
      throw new ApiError(400, 'Cannot change your own status');
    }

    const user = await prisma.user.update({
      where: { id },
      data: { status },
      select: {
        id: true,
        email: true,
        status: true,
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

// GET /api/v1/admin/stats
router.get('/stats', async (req: AuthRequest, res, next) => {
  try {
    const totalUsers = await prisma.user.count();
    const activeUsers = await prisma.user.count({ where: { status: 'ACTIVE' } });
    const totalOrders = await prisma.order.count();
    const activePositions = await prisma.position.count({ where: { status: 'OPEN' } });
    const totalTrades = await prisma.trade.count();

    const pendingInvitations = await prisma.invitationCode.count({
      where: {
        usedAt: null,
        expiresAt: { gt: new Date() },
      },
    });

    const usedInvitations = await prisma.invitationCode.count({
      where: { usedAt: { not: null } },
    });

    res.json({
      success: true,
      data: {
        users: {
          total: totalUsers,
          active: activeUsers,
        },
        trading: {
          totalOrders,
          activePositions,
          totalTrades,
        },
        invitations: {
          pending: pendingInvitations,
          used: usedInvitations,
        },
      },
    });
  } catch (error) {
    next(error);
  }
});

export default router;
