<?php
// public/index.php — single entry point. Routes /api/* to the corresponding
// JSON endpoint; everything else renders the HTML dashboard.
declare(strict_types=1);

require __DIR__ . '/../includes/data_source.php';
require __DIR__ . '/../includes/render.php';

$path = parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH) ?? '/';

// ── Static assets ───────────────────────────────────────────────────────────
// Serve files under /assets/* directly when this script runs as the router
// for the PHP built-in server (php -S host:port public/index.php).
if (strpos($path, '/assets/') === 0) {
    $relative = substr($path, strlen('/assets/'));
    // Disallow any directory traversal.
    if (strpos($relative, '..') !== false || $relative === '') {
        http_response_code(404);
        return;
    }
    $file = realpath(__DIR__ . '/../assets/' . $relative);
    $base = realpath(__DIR__ . '/../assets');
    if ($file === false || $base === false || strpos($file, $base) !== 0 || !is_file($file)) {
        http_response_code(404);
        return;
    }
    $ext = strtolower(pathinfo($file, PATHINFO_EXTENSION));
    $mime = [
        'css'  => 'text/css; charset=utf-8',
        'js'   => 'application/javascript; charset=utf-8',
        'svg'  => 'image/svg+xml',
        'png'  => 'image/png',
        'ico'  => 'image/x-icon',
        'json' => 'application/json',
    ][$ext] ?? 'application/octet-stream';
    header('Content-Type: ' . $mime);
    header('Cache-Control: public, max-age=300');
    readfile($file);
    return;
}

// Static assets when running under `php -S` (built-in server already maps
// existing files; we only need the explicit /api/* dispatcher).
if (strpos($path, '/api/') === 0) {
    $endpoint = basename($path);
    $candidate = __DIR__ . '/../api/' . $endpoint;
    if (preg_match('/^[a-z_]+\.php$/', $endpoint) && is_file($candidate)) {
        require $candidate;
        return;
    }
    vt_json_response(['error' => 'unknown endpoint'], 404);
    return;
}

$view = $_GET['view'] ?? 'dashboard';

switch ($view) {
    case 'trades':
        vt_render_layout('Trades', function () {
            ?>
            <section class="vt-card">
                <header class="vt-card__head"><h2>Trade history</h2></header>
                <div class="vt-card__body">
                    <table class="vt-table" id="vt-trades">
                        <thead>
                            <tr>
                                <th>Time</th><th>Symbol</th><th>Side</th>
                                <th class="num">Qty</th><th class="num">Entry</th>
                                <th class="num">Exit</th><th class="num">PnL</th>
                            </tr>
                        </thead>
                        <tbody><tr><td colspan="7" class="vt-empty">loading…</td></tr></tbody>
                    </table>
                </div>
            </section>
            <?php
        });
        break;

    case 'about':
        vt_render_layout('About', function () {
            $cfg = require __DIR__ . '/../includes/config.php';
            ?>
            <section class="vt-card">
                <header class="vt-card__head"><h2>About</h2></header>
                <div class="vt-card__body">
                    <p><strong><?= vt_h($cfg['app_name']) ?></strong> —
                       веб-интерфейс к торговому движку Crypto ML Trader.</p>
                    <p>Версия: <code><?= vt_h($cfg['app_version']) ?></code></p>
                    <p>Источник данных: <code><?= vt_h($cfg['data_dir']) ?></code></p>
                    <p>Документация: см. <code>web-version-php/README.md</code></p>
                </div>
            </section>
            <?php
        });
        break;

    case 'dashboard':
    default:
        vt_render_layout('Dashboard', function () {
            ?>
            <section class="vt-grid">
                <div class="vt-card vt-card--span2">
                    <header class="vt-card__head">
                        <h2 id="vt-symbol">—</h2>
                        <span class="vt-price" id="vt-price">—</span>
                        <span class="vt-change" id="vt-change">—</span>
                    </header>
                    <div class="vt-card__body">
                        <canvas id="vt-chart" height="320"></canvas>
                    </div>
                </div>

                <div class="vt-card">
                    <header class="vt-card__head"><h2>Account</h2></header>
                    <div class="vt-card__body vt-stats">
                        <div><span>Balance</span><b id="vt-balance">—</b></div>
                        <div><span>Equity</span><b id="vt-equity">—</b></div>
                        <div><span>PnL (open)</span><b id="vt-pnl-open">—</b></div>
                        <div><span>PnL (total)</span><b id="vt-pnl-total">—</b></div>
                        <div><span>Drawdown</span><b id="vt-drawdown">—</b></div>
                    </div>
                </div>

                <div class="vt-card">
                    <header class="vt-card__head"><h2>Open positions</h2></header>
                    <div class="vt-card__body">
                        <table class="vt-table" id="vt-positions">
                            <thead>
                                <tr>
                                    <th>Symbol</th><th>Side</th>
                                    <th class="num">Qty</th><th class="num">Entry</th>
                                    <th class="num">Mark</th><th class="num">PnL</th>
                                </tr>
                            </thead>
                            <tbody><tr><td colspan="6" class="vt-empty">no open positions</td></tr></tbody>
                        </table>
                    </div>
                </div>
            </section>
            <?php
        });
        break;
}
