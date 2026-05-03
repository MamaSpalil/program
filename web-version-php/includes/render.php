<?php
// includes/render.php — shared layout helpers.
declare(strict_types=1);

function vt_h(string $s): string
{
    return htmlspecialchars($s, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

function vt_render_layout(string $title, callable $body): void
{
    $cfg = require __DIR__ . '/config.php';
    ?><!doctype html>
<html lang="ru" data-theme="dark">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title><?= vt_h($title) ?> — <?= vt_h($cfg['app_name']) ?></title>
    <link rel="stylesheet" href="/assets/css/app.css">
    <script>
        // Apply persisted theme as early as possible to avoid a flash of the
        // wrong theme on initial paint.
        (function () {
            try {
                var t = localStorage.getItem('vt-theme');
                if (!t) {
                    t = window.matchMedia('(prefers-color-scheme: light)').matches
                        ? 'light' : 'dark';
                }
                document.documentElement.setAttribute('data-theme', t);
            } catch (e) { /* localStorage may be disabled */ }
        })();
    </script>
</head>
<body>
    <header class="vt-header">
        <div class="vt-header__brand">
            <span class="vt-logo">VT</span>
            <span class="vt-title"><?= vt_h($cfg['app_name']) ?></span>
            <span class="vt-version">v<?= vt_h($cfg['app_version']) ?></span>
        </div>
        <nav class="vt-nav">
            <a href="/" class="vt-nav__link">Dashboard</a>
            <a href="/?view=trades" class="vt-nav__link">Trades</a>
            <a href="/?view=about" class="vt-nav__link">About</a>
        </nav>
        <div class="vt-header__actions">
            <button id="vt-theme-toggle" class="vt-btn" type="button"
                    aria-label="Toggle theme" title="Light / Dark theme">
                <span class="vt-theme-icon"></span>
            </button>
        </div>
    </header>

    <main class="vt-main">
        <?php $body(); ?>
    </main>

    <footer class="vt-footer">
        <span>© <?= date('Y') ?> VT</span>
        <span class="vt-footer__status" id="vt-status">connecting…</span>
    </footer>

    <script>window.VT_POLL_MS = <?= (int)$cfg['poll_ms'] ?>;</script>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"
            integrity="sha384-9MhbyIRcBVQiiC7FSd7T38oJNj2Zh+EfxS7/vjhBi4OOT78NlHSnzM31EZRWR1LZ"
            crossorigin="anonymous"></script>
    <script src="/assets/js/app.js"></script>
</body>
</html><?php
}
