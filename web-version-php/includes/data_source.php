<?php
// includes/data_source.php — read-only access to engine JSON snapshots.
declare(strict_types=1);

/**
 * Load a JSON snapshot file from the configured data directory.
 * Returns an associative array, or the supplied default on missing/invalid file.
 */
function vt_load_snapshot(string $filename, $default = null)
{
    static $cfg = null;
    if ($cfg === null) {
        $cfg = require __DIR__ . '/config.php';
    }

    // Whitelist filename to a known set — no user input flows into the path.
    $allowed = [
        $cfg['state_file'],
        $cfg['candles_file'],
        $cfg['trades_file'],
    ];
    if (!in_array($filename, $allowed, true)) {
        return $default;
    }

    $path = $cfg['data_dir'] . DIRECTORY_SEPARATOR . $filename;
    if (!is_file($path) || !is_readable($path)) {
        return $default;
    }

    $raw = @file_get_contents($path);
    if ($raw === false || $raw === '') {
        return $default;
    }

    $decoded = json_decode($raw, true);
    if (!is_array($decoded)) {
        return $default;
    }
    return $decoded;
}

/**
 * Emit a JSON response with no-cache headers.
 */
function vt_json_response($payload, int $status = 200): void
{
    http_response_code($status);
    header('Content-Type: application/json; charset=utf-8');
    header('Cache-Control: no-store, no-cache, must-revalidate, max-age=0');
    header('Pragma: no-cache');
    echo json_encode($payload, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
}
