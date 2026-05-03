<?php
// api/snapshot.php — current account / market snapshot.
declare(strict_types=1);
require_once __DIR__ . '/../includes/data_source.php';

$cfg = require __DIR__ . '/../includes/config.php';

$state = vt_load_snapshot($cfg['state_file'], null);
if ($state === null) {
    // Empty placeholder so the UI can render without an engine running.
    $state = [
        'symbol'      => null,
        'price'       => null,
        'change_pct'  => null,
        'connected'   => false,
        'account'     => [
            'balance'     => null,
            'equity'      => null,
            'pnl_open'    => null,
            'pnl_total'   => null,
            'drawdown'    => null,
        ],
        'positions'   => [],
        'updated_at'  => null,
    ];
}

vt_json_response($state);
