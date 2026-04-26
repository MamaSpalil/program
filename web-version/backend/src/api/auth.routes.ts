import { Router } from 'express';
import { z } from 'zod';
import AuthService from '../auth/AuthService';
import { authRateLimiter } from '../middleware/rateLimiter';
import { ApiError } from '../middleware/errorHandler';

const router = Router();

// Validation schemas
const registerSchema = z.object({
  email: z.string().email(),
  password: z.string().min(8),
  firstName: z.string().optional(),
  lastName: z.string().optional(),
});

const loginSchema = z.object({
  email: z.string().email(),
  password: z.string(),
});

const refreshSchema = z.object({
  refreshToken: z.string(),
});

const changePasswordSchema = z.object({
  oldPassword: z.string(),
  newPassword: z.string().min(8),
});

// POST /api/v1/auth/register
router.post('/register', authRateLimiter, async (req, res, next) => {
  try {
    const validated = registerSchema.parse(req.body);
    const result = await AuthService.register(
      validated.email,
      validated.password,
      validated.firstName,
      validated.lastName
    );

    res.status(201).json({
      success: true,
      data: result,
    });
  } catch (error) {
    if (error instanceof z.ZodError) {
      next(new ApiError(400, 'Validation error: ' + error.errors[0].message));
    } else {
      next(error);
    }
  }
});

// POST /api/v1/auth/login
router.post('/login', authRateLimiter, async (req, res, next) => {
  try {
    const validated = loginSchema.parse(req.body);
    const result = await AuthService.login(validated.email, validated.password);

    res.json({
      success: true,
      data: result,
    });
  } catch (error) {
    if (error instanceof z.ZodError) {
      next(new ApiError(400, 'Validation error: ' + error.errors[0].message));
    } else {
      next(error);
    }
  }
});

// POST /api/v1/auth/refresh
router.post('/refresh', async (req, res, next) => {
  try {
    const validated = refreshSchema.parse(req.body);
    const result = await AuthService.refreshToken(validated.refreshToken);

    res.json({
      success: true,
      data: result,
    });
  } catch (error) {
    if (error instanceof z.ZodError) {
      next(new ApiError(400, 'Validation error: ' + error.errors[0].message));
    } else {
      next(error);
    }
  }
});

// POST /api/v1/auth/logout
router.post('/logout', async (req, res) => {
  // With JWT, logout is handled client-side by removing the token
  // In future, could implement token blacklisting here
  res.json({
    success: true,
    message: 'Logged out successfully',
  });
});

export default router;
