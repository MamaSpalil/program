import bcrypt from 'bcrypt';
import jwt from 'jsonwebtoken';
import { PrismaClient } from '@prisma/client';
import config from '../config';
import { ApiError } from '../middleware/errorHandler';
import logger from '../utils/logger';
import InvitationService from './InvitationService';

const prisma = new PrismaClient();

export class AuthService {
  async register(email: string, password: string, invitationCode: string, firstName?: string, lastName?: string) {
    // Check if this is the first user
    const isFirstUser = await InvitationService.isFirstUser();

    // If not first user, validate invitation code
    if (!isFirstUser) {
      await InvitationService.validateInvitation(invitationCode, email);
    }

    // Check if user already exists
    const existingUser = await prisma.user.findUnique({
      where: { email },
    });

    if (existingUser) {
      throw new ApiError(400, 'User with this email already exists');
    }

    // Hash password
    const passwordHash = await bcrypt.hash(password, 12);

    // Create user with default settings
    // First user is automatically admin
    const user = await prisma.user.create({
      data: {
        email,
        passwordHash,
        firstName,
        lastName,
        role: isFirstUser ? 'ADMIN' : 'USER',
        settings: {
          create: {
            paperTradingMode: true, // Start in paper trading by default
          },
        },
      },
      select: {
        id: true,
        email: true,
        firstName: true,
        lastName: true,
        role: true,
        createdAt: true,
      },
    });

    // Mark invitation as used (if not first user)
    if (!isFirstUser) {
      await InvitationService.markInvitationUsed(invitationCode, user.id);
    }

    logger.info(`New user registered: ${email} (${isFirstUser ? 'ADMIN' : 'USER'})`);

    // Generate tokens
    const tokens = this.generateTokens(user.id, user.email, user.role);

    return {
      user,
      ...tokens,
    };
  }

  async login(email: string, password: string) {
    // Find user
    const user = await prisma.user.findUnique({
      where: { email },
      select: {
        id: true,
        email: true,
        passwordHash: true,
        firstName: true,
        lastName: true,
        role: true,
        status: true,
        twoFactorEnabled: true,
      },
    });

    if (!user) {
      throw new ApiError(401, 'Invalid credentials');
    }

    // Check if user is active
    if (user.status !== 'ACTIVE') {
      throw new ApiError(401, 'Account is not active');
    }

    // Verify password
    const isPasswordValid = await bcrypt.compare(password, user.passwordHash);

    if (!isPasswordValid) {
      throw new ApiError(401, 'Invalid credentials');
    }

    // Check if 2FA is enabled
    if (user.twoFactorEnabled) {
      // Return a temporary token that requires 2FA verification
      return {
        requiresTwoFactor: true,
        tempToken: this.generateTempToken(user.id),
      };
    }

    // Update last login
    await prisma.user.update({
      where: { id: user.id },
      data: { lastLoginAt: new Date() },
    });

    logger.info(`User logged in: ${email}`);

    // Generate tokens
    const tokens = this.generateTokens(user.id, user.email, user.role);

    return {
      user: {
        id: user.id,
        email: user.email,
        firstName: user.firstName,
        lastName: user.lastName,
        role: user.role,
      },
      ...tokens,
    };
  }

  async refreshToken(refreshToken: string) {
    try {
      const decoded = jwt.verify(refreshToken, config.jwtSecret) as {
        userId: string;
        type: string;
      };

      if (decoded.type !== 'refresh') {
        throw new ApiError(401, 'Invalid token type');
      }

      const user = await prisma.user.findUnique({
        where: { id: decoded.userId },
        select: { id: true, email: true, role: true, status: true },
      });

      if (!user || user.status !== 'ACTIVE') {
        throw new ApiError(401, 'User not found or inactive');
      }

      // Generate new tokens
      const tokens = this.generateTokens(user.id, user.email, user.role);

      return tokens;
    } catch (error) {
      if (error instanceof jwt.JsonWebTokenError || error instanceof jwt.TokenExpiredError) {
        throw new ApiError(401, 'Invalid or expired refresh token');
      }
      throw error;
    }
  }

  async changePassword(userId: string, oldPassword: string, newPassword: string) {
    const user = await prisma.user.findUnique({
      where: { id: userId },
      select: { passwordHash: true },
    });

    if (!user) {
      throw new ApiError(404, 'User not found');
    }

    // Verify old password
    const isPasswordValid = await bcrypt.compare(oldPassword, user.passwordHash);

    if (!isPasswordValid) {
      throw new ApiError(401, 'Current password is incorrect');
    }

    // Hash new password
    const newPasswordHash = await bcrypt.hash(newPassword, 12);

    // Update password
    await prisma.user.update({
      where: { id: userId },
      data: { passwordHash: newPasswordHash },
    });

    logger.info(`Password changed for user: ${userId}`);
  }

  private generateTokens(userId: string, email: string, role: string) {
    // @ts-ignore - jwt.sign types have issues with string | number for expiresIn
    const accessToken = jwt.sign(
      { userId, email, role, type: 'access' },
      config.jwtSecret,
      { expiresIn: config.jwtExpiresIn }
    );

    // @ts-ignore - jwt.sign types have issues with string | number for expiresIn
    const refreshToken = jwt.sign(
      { userId, type: 'refresh' },
      config.jwtSecret,
      { expiresIn: config.jwtRefreshExpiresIn }
    );

    return {
      accessToken,
      refreshToken,
      expiresIn: config.jwtExpiresIn,
    };
  }

  private generateTempToken(userId: string): string {
    return jwt.sign(
      { userId, type: 'temp' },
      config.jwtSecret,
      { expiresIn: '10m' }
    );
  }
}

export default new AuthService();
