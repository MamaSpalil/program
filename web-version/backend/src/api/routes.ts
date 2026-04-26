import { Router } from 'express';
import { authenticate } from '../middleware/auth';
import authRoutes from './auth.routes';
import userRoutes from './user.routes';
import exchangeRoutes from './exchange.routes';
import tradingRoutes from './trading.routes';
import adminRoutes from './admin.routes';
import analysisRoutes from './analysis.routes';

const router = Router();

// Public routes
router.use('/auth', authRoutes);

// Protected routes (require authentication)
router.use('/users', authenticate, userRoutes);
router.use('/exchanges', authenticate, exchangeRoutes);
router.use('/trading', authenticate, tradingRoutes);
router.use('/admin', authenticate, adminRoutes);
router.use('/analysis', authenticate, analysisRoutes);

export default router;
