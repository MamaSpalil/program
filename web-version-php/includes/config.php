<?php
// includes/config.php — paths and runtime settings for the VT Web dashboard.
declare(strict_types=1);

// Base directory where the C++ engine writes JSON snapshots.
// Override with the VT_DATA_DIR environment variable.
$dataDir = getenv('VT_DATA_DIR');
if ($dataDir === false || $dataDir === '') {
    // Default: web-version-php/data (sibling of public/, includes/, …).
    $dataDir = realpath(__DIR__ . '/..') . DIRECTORY_SEPARATOR . 'data';
}

return [
    'data_dir'      => $dataDir,
    'state_file'    => 'state.json',
    'candles_file'  => 'candles.json',
    'trades_file'   => 'trades.json',
    'app_name'      => 'VT — Virtual Trade System',
    'app_version'   => '2.8.0',
    // Refresh interval for client-side polling (ms).
    'poll_ms'       => 1000,
];
