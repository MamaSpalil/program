<?php
// api/trades.php — closed trade history.
declare(strict_types=1);
require_once __DIR__ . '/../includes/data_source.php';

$cfg = require __DIR__ . '/../includes/config.php';
$trades = vt_load_snapshot($cfg['trades_file'], ['trades' => []]);
vt_json_response($trades);
