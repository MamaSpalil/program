<?php
// api/candles.php — recent OHLCV candles for the chart.
declare(strict_types=1);
require_once __DIR__ . '/../includes/data_source.php';

$cfg = require __DIR__ . '/../includes/config.php';
$candles = vt_load_snapshot($cfg['candles_file'], ['symbol' => null, 'interval' => null, 'candles' => []]);
vt_json_response($candles);
