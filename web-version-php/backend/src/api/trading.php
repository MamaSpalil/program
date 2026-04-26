<?php

/**
 * Trading API Routes
 */

use CryptoTrader\Database\Connection;
use CryptoTrader\Auth\Auth;

function handleTradingRequest($action, $method, $input)
{
    // Verify authentication
    $auth = new Auth();
    $session = $auth->validateSession();

    if (!$session) {
        http_response_code(401);
        echo json_encode(['error' => 'Unauthorized']);
        return;
    }

    $db = Connection::getInstance();
    $userId = $session['user_id'];

    // Parse action
    $parts = explode('/', $action);
    $endpoint = $parts[0];

    try {
        switch ($endpoint) {
            case 'order':
                if ($method === 'POST') {
                    // Place order
                    $symbol = $input['symbol'] ?? '';
                    $side = $input['side'] ?? '';
                    $type = $input['type'] ?? 'MARKET';
                    $quantity = floatval($input['quantity'] ?? 0);
                    $price = isset($input['price']) ? floatval($input['price']) : null;

                    // Validate input
                    if (empty($symbol) || empty($side) || $quantity <= 0) {
                        throw new \Exception('Invalid order parameters');
                    }

                    // Insert order into database
                    $sql = 'INSERT INTO orders (user_id, exchange_name, symbol, order_type, side, quantity, price, status, created_at)
                            VALUES (?, ?, ?, ?, ?, ?, ?, ?, GETDATE())';

                    $db->execute($sql, [
                        $userId,
                        'Binance',
                        $symbol,
                        $type,
                        $side,
                        $quantity,
                        $price,
                        'PENDING'
                    ]);

                    $orderId = $db->lastInsertId();

                    echo json_encode([
                        'success' => true,
                        'data' => [
                            'order_id' => $orderId,
                            'status' => 'PENDING'
                        ]
                    ]);
                } else {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                }
                break;

            case 'orders':
                if ($method !== 'GET') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                // Get open orders
                $sql = 'SELECT order_id, symbol, order_type, side, quantity, price, status, created_at
                        FROM orders
                        WHERE user_id = ? AND status IN (?, ?)
                        ORDER BY created_at DESC';

                $stmt = $db->execute($sql, [$userId, 'PENDING', 'OPEN']);
                $orders = $db->fetchAll($stmt);

                echo json_encode([
                    'success' => true,
                    'data' => $orders
                ]);
                break;

            case 'cancel':
                if ($method !== 'POST') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                $orderId = $parts[1] ?? null;

                if (!$orderId) {
                    throw new \Exception('Order ID is required');
                }

                // Update order status
                $sql = 'UPDATE orders SET status = ?, updated_at = GETDATE()
                        WHERE order_id = ? AND user_id = ?';

                $db->execute($sql, ['CANCELLED', $orderId, $userId]);

                echo json_encode([
                    'success' => true,
                    'message' => 'Order cancelled'
                ]);
                break;

            case 'positions':
                if ($method !== 'GET') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                // Get open positions
                $sql = 'SELECT position_id, symbol, side, entry_price, quantity, leverage,
                        unrealized_pnl, realized_pnl, stop_loss, take_profit, opened_at
                        FROM positions
                        WHERE user_id = ? AND status = ?
                        ORDER BY opened_at DESC';

                $stmt = $db->execute($sql, [$userId, 'OPEN']);
                $positions = $db->fetchAll($stmt);

                echo json_encode([
                    'success' => true,
                    'data' => $positions
                ]);
                break;

            case 'history':
                if ($method !== 'GET') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                $limit = intval($_GET['limit'] ?? 50);

                // Get trade history
                $sql = 'SELECT trade_id, symbol, side, price, quantity, fee, realized_pnl, executed_at
                        FROM trades
                        WHERE user_id = ?
                        ORDER BY executed_at DESC
                        OFFSET 0 ROWS FETCH NEXT ? ROWS ONLY';

                $stmt = $db->execute($sql, [$userId, $limit]);
                $trades = $db->fetchAll($stmt);

                echo json_encode([
                    'success' => true,
                    'data' => $trades
                ]);
                break;

            case 'portfolio':
                if ($method !== 'GET') {
                    http_response_code(405);
                    echo json_encode(['error' => 'Method not allowed']);
                    return;
                }

                // Get portfolio summary using stored procedure
                $sql = 'EXEC sp_get_portfolio_summary @user_id = ?';
                $stmt = $db->execute($sql, [$userId]);
                $summary = $db->fetch($stmt);

                echo json_encode([
                    'success' => true,
                    'data' => $summary
                ]);
                break;

            default:
                http_response_code(404);
                echo json_encode(['error' => 'Endpoint not found']);
        }
    } catch (\Exception $e) {
        http_response_code(500);
        echo json_encode(['error' => $e->getMessage()]);
    }
}
