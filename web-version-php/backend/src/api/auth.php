<?php

/**
 * Auth API Routes
 */

use CryptoTrader\Auth\Auth;

function handleAuthRequest($action, $method, $input)
{
    $auth = new Auth();

    switch ($action) {
        case 'login':
            if ($method !== 'POST') {
                http_response_code(405);
                echo json_encode(['error' => 'Method not allowed']);
                return;
            }

            try {
                $result = $auth->login(
                    $input['username'] ?? '',
                    $input['password'] ?? '',
                    $input['remember_me'] ?? false
                );

                echo json_encode([
                    'success' => true,
                    'data' => $result
                ]);
            } catch (\Exception $e) {
                http_response_code(401);
                echo json_encode(['error' => $e->getMessage()]);
            }
            break;

        case 'register':
            if ($method !== 'POST') {
                http_response_code(405);
                echo json_encode(['error' => 'Method not allowed']);
                return;
            }

            try {
                $userId = $auth->register(
                    $input['username'] ?? '',
                    $input['email'] ?? '',
                    $input['password'] ?? '',
                    $input['full_name'] ?? null
                );

                echo json_encode([
                    'success' => true,
                    'data' => ['user_id' => $userId]
                ]);
            } catch (\Exception $e) {
                http_response_code(400);
                echo json_encode(['error' => $e->getMessage()]);
            }
            break;

        case 'logout':
            if ($method !== 'POST') {
                http_response_code(405);
                echo json_encode(['error' => 'Method not allowed']);
                return;
            }

            try {
                $auth->logout();
                echo json_encode(['success' => true]);
            } catch (\Exception $e) {
                http_response_code(400);
                echo json_encode(['error' => $e->getMessage()]);
            }
            break;

        case 'me':
            if ($method !== 'GET') {
                http_response_code(405);
                echo json_encode(['error' => 'Method not allowed']);
                return;
            }

            try {
                $session = $auth->validateSession();
                if (!$session) {
                    http_response_code(401);
                    echo json_encode(['error' => 'Unauthorized']);
                    return;
                }

                $user = $auth->getUserById($session['user_id']);
                unset($user['password_hash']);

                echo json_encode([
                    'success' => true,
                    'data' => $user
                ]);
            } catch (\Exception $e) {
                http_response_code(500);
                echo json_encode(['error' => $e->getMessage()]);
            }
            break;

        default:
            http_response_code(404);
            echo json_encode(['error' => 'Endpoint not found']);
    }
}
