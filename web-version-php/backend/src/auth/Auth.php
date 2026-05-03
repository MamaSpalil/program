<?php

namespace CryptoTrader\Auth;

use CryptoTrader\Database\Connection;

/**
 * Authentication Service
 */
class Auth
{
    private $db;
    private $config;

    public function __construct()
    {
        $this->db = Connection::getInstance();
        $this->config = require __DIR__ . '/../../config/config.php';
    }

    /**
     * Register new user
     */
    public function register($username, $email, $password, $fullName = null)
    {
        // Validate input
        if (empty($username) || empty($email) || empty($password)) {
            throw new \Exception('Username, email, and password are required');
        }

        if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
            throw new \Exception('Invalid email format');
        }

        if (strlen($password) < 8) {
            throw new \Exception('Password must be at least 8 characters');
        }

        // Check if user exists
        $sql = 'SELECT user_id FROM users WHERE username = ? OR email = ?';
        $stmt = $this->db->execute($sql, [$username, $email]);
        if ($this->db->fetch($stmt)) {
            throw new \Exception('Username or email already exists');
        }

        // Hash password
        $passwordHash = password_hash($password, $this->config['security']['hash_algo'], [
            'cost' => $this->config['security']['hash_cost']
        ]);

        // Insert user
        $sql = 'INSERT INTO users (username, email, password_hash, full_name, created_at, updated_at)
                VALUES (?, ?, ?, ?, GETDATE(), GETDATE())';
        $this->db->execute($sql, [$username, $email, $passwordHash, $fullName]);

        return $this->db->lastInsertId();
    }

    /**
     * Login user
     */
    public function login($username, $password, $rememberMe = false)
    {
        // Find user
        $sql = 'SELECT user_id, username, email, password_hash, is_active, two_factor_enabled
                FROM users WHERE username = ? OR email = ?';
        $stmt = $this->db->execute($sql, [$username, $username]);
        $user = $this->db->fetch($stmt);

        if (!$user) {
            throw new \Exception('Invalid credentials');
        }

        // Check if active
        if (!$user['is_active']) {
            throw new \Exception('Account is disabled');
        }

        // Verify password
        if (!password_verify($password, $user['password_hash'])) {
            throw new \Exception('Invalid credentials');
        }

        // Update last login
        $updateSql = 'UPDATE users SET last_login_at = GETDATE() WHERE user_id = ?';
        $this->db->execute($updateSql, [$user['user_id']]);

        // Create session
        $sessionId = $this->createSession($user['user_id'], $rememberMe);

        return [
            'user_id' => $user['user_id'],
            'username' => $user['username'],
            'email' => $user['email'],
            'session_id' => $sessionId,
            'two_factor_required' => $user['two_factor_enabled']
        ];
    }

    /**
     * Create session
     */
    private function createSession($userId, $rememberMe = false)
    {
        $sessionId = bin2hex(random_bytes(32));
        $lifetime = $rememberMe ? 30 * 24 * 60 * 60 : $this->config['session']['lifetime'];

        $sql = 'INSERT INTO sessions (session_id, user_id, ip_address, user_agent, created_at, expires_at, last_activity)
                VALUES (?, ?, ?, ?, GETDATE(), DATEADD(second, ?, GETDATE()), GETDATE())';

        $this->db->execute($sql, [
            $sessionId,
            $userId,
            $_SERVER['REMOTE_ADDR'] ?? null,
            $_SERVER['HTTP_USER_AGENT'] ?? null,
            $lifetime
        ]);

        // Set session cookie
        session_start();
        $_SESSION['session_id'] = $sessionId;
        $_SESSION['user_id'] = $userId;

        return $sessionId;
    }

    /**
     * Validate session
     */
    public function validateSession($sessionId = null)
    {
        if ($sessionId === null) {
            session_start();
            $sessionId = $_SESSION['session_id'] ?? null;
        }

        if (!$sessionId) {
            return false;
        }

        $sql = 'SELECT s.user_id, s.expires_at, u.username, u.email, u.is_active
                FROM sessions s
                JOIN users u ON s.user_id = u.user_id
                WHERE s.session_id = ? AND s.expires_at > GETDATE()';

        $stmt = $this->db->execute($sql, [$sessionId]);
        $session = $this->db->fetch($stmt);

        if (!$session || !$session['is_active']) {
            return false;
        }

        // Update last activity
        $updateSql = 'UPDATE sessions SET last_activity = GETDATE() WHERE session_id = ?';
        $this->db->execute($updateSql, [$sessionId]);

        return $session;
    }

    /**
     * Logout user
     */
    public function logout($sessionId = null)
    {
        if ($sessionId === null) {
            session_start();
            $sessionId = $_SESSION['session_id'] ?? null;
        }

        if ($sessionId) {
            $sql = 'DELETE FROM sessions WHERE session_id = ?';
            $this->db->execute($sql, [$sessionId]);
        }

        // Destroy PHP session
        session_destroy();
        return true;
    }

    /**
     * Change password
     */
    public function changePassword($userId, $currentPassword, $newPassword)
    {
        if (strlen($newPassword) < 8) {
            throw new \Exception('Password must be at least 8 characters');
        }

        // Get current password hash
        $sql = 'SELECT password_hash FROM users WHERE user_id = ?';
        $stmt = $this->db->execute($sql, [$userId]);
        $user = $this->db->fetch($stmt);

        if (!$user) {
            throw new \Exception('User not found');
        }

        // Verify current password
        if (!password_verify($currentPassword, $user['password_hash'])) {
            throw new \Exception('Current password is incorrect');
        }

        // Hash new password
        $newPasswordHash = password_hash($newPassword, $this->config['security']['hash_algo'], [
            'cost' => $this->config['security']['hash_cost']
        ]);

        // Update password
        $updateSql = 'UPDATE users SET password_hash = ?, updated_at = GETDATE() WHERE user_id = ?';
        $this->db->execute($updateSql, [$newPasswordHash, $userId]);

        return true;
    }

    /**
     * Get user by ID
     */
    public function getUserById($userId)
    {
        $sql = 'SELECT user_id, username, email, full_name, is_active, is_admin,
                email_verified, two_factor_enabled, created_at, last_login_at
                FROM users WHERE user_id = ?';

        $stmt = $this->db->execute($sql, [$userId]);
        return $this->db->fetch($stmt);
    }
}
