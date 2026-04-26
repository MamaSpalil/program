import crypto from 'crypto';
import { PrismaClient } from '@prisma/client';
import { ApiError } from '../middleware/errorHandler';
import logger from '../utils/logger';

const prisma = new PrismaClient();

export class InvitationService {
  /**
   * Generate invitation code for a specific email (admin only)
   */
  async generateInvitation(adminId: string, email: string, expiresInDays: number = 7): Promise<{
    code: string;
    email: string;
    expiresAt: Date;
  }> {
    // Verify admin has ADMIN role
    const admin = await prisma.user.findUnique({
      where: { id: adminId },
      select: { role: true },
    });

    if (!admin || admin.role !== 'ADMIN') {
      throw new ApiError(403, 'Only administrators can generate invitation codes');
    }

    // Check if email already has a user
    const existingUser = await prisma.user.findUnique({
      where: { email },
    });

    if (existingUser) {
      throw new ApiError(400, 'User with this email already exists');
    }

    // Check if there's already an active invitation for this email
    const existingInvitation = await prisma.invitationCode.findFirst({
      where: {
        email,
        expiresAt: { gt: new Date() },
        usedAt: null,
      },
    });

    if (existingInvitation) {
      throw new ApiError(400, 'Active invitation for this email already exists');
    }

    // Generate unique code
    const code = this.generateUniqueCode();
    const expiresAt = new Date();
    expiresAt.setDate(expiresAt.getDate() + expiresInDays);

    // Create invitation
    const invitation = await prisma.invitationCode.create({
      data: {
        code,
        email,
        createdById: adminId,
        expiresAt,
      },
    });

    logger.info(`Invitation code generated for ${email} by admin ${adminId}`);

    return {
      code: invitation.code,
      email: invitation.email,
      expiresAt: invitation.expiresAt,
    };
  }

  /**
   * Validate invitation code
   */
  async validateInvitation(code: string, email: string): Promise<boolean> {
    const invitation = await prisma.invitationCode.findFirst({
      where: {
        code,
        email,
      },
    });

    if (!invitation) {
      throw new ApiError(400, 'Invalid invitation code or email');
    }

    if (invitation.usedAt) {
      throw new ApiError(400, 'Invitation code has already been used');
    }

    if (new Date() > invitation.expiresAt) {
      throw new ApiError(400, 'Invitation code has expired');
    }

    return true;
  }

  /**
   * Mark invitation as used
   */
  async markInvitationUsed(code: string, userId: string): Promise<void> {
    await prisma.invitationCode.update({
      where: { code },
      data: {
        usedById: userId,
        usedAt: new Date(),
      },
    });

    logger.info(`Invitation code ${code} used by user ${userId}`);
  }

  /**
   * Get all invitations created by admin
   */
  async getInvitationsByAdmin(adminId: string): Promise<Array<{
    id: string;
    code: string;
    email: string;
    expiresAt: Date;
    usedAt: Date | null;
    createdAt: Date;
  }>> {
    const invitations = await prisma.invitationCode.findMany({
      where: { createdById: adminId },
      orderBy: { createdAt: 'desc' },
      select: {
        id: true,
        code: true,
        email: true,
        expiresAt: true,
        usedAt: true,
        createdAt: true,
      },
    });

    return invitations;
  }

  /**
   * Revoke (delete) an invitation
   */
  async revokeInvitation(adminId: string, invitationId: string): Promise<void> {
    // Verify admin owns this invitation
    const invitation = await prisma.invitationCode.findFirst({
      where: {
        id: invitationId,
        createdById: adminId,
      },
    });

    if (!invitation) {
      throw new ApiError(404, 'Invitation not found or not owned by this admin');
    }

    if (invitation.usedAt) {
      throw new ApiError(400, 'Cannot revoke an already used invitation');
    }

    await prisma.invitationCode.delete({
      where: { id: invitationId },
    });

    logger.info(`Invitation ${invitationId} revoked by admin ${adminId}`);
  }

  /**
   * Ensure first user is automatically admin
   */
  async ensureFirstUserIsAdmin(): Promise<void> {
    const userCount = await prisma.user.count();

    if (userCount === 0) {
      logger.info('No users exist yet - first registration will be admin');
    }
  }

  /**
   * Check if this is the first user (for auto-admin)
   */
  async isFirstUser(): Promise<boolean> {
    const userCount = await prisma.user.count();
    return userCount === 0;
  }

  /**
   * Generate unique invitation code
   */
  private generateUniqueCode(): string {
    return crypto.randomBytes(16).toString('hex').toUpperCase();
  }
}

export default new InvitationService();
