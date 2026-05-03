// VT Web — theme switcher + polling client.
'use strict';

(function () {
    // ── Theme switcher ──────────────────────────────────────────────────────
    const root = document.documentElement;
    const toggle = document.getElementById('vt-theme-toggle');
    if (toggle) {
        toggle.addEventListener('click', () => {
            const current = root.getAttribute('data-theme') === 'light' ? 'light' : 'dark';
            const next = current === 'light' ? 'dark' : 'light';
            root.setAttribute('data-theme', next);
            try { localStorage.setItem('vt-theme', next); } catch (e) { /* ignore */ }
            // Re-tint the chart canvas if present.
            window.dispatchEvent(new CustomEvent('vt:theme-change', { detail: next }));
        });
    }

    // ── Helpers ─────────────────────────────────────────────────────────────
    const fmt = {
        price(v) {
            if (v == null || !isFinite(v)) return '—';
            const abs = Math.abs(v);
            const digits = abs >= 100 ? 2 : abs >= 1 ? 4 : 8;
            return Number(v).toFixed(digits);
        },
        money(v) {
            if (v == null || !isFinite(v)) return '—';
            return Number(v).toFixed(2);
        },
        pct(v) {
            if (v == null || !isFinite(v)) return '—';
            const sign = v >= 0 ? '+' : '';
            return sign + Number(v).toFixed(2) + '%';
        },
        time(ts) {
            if (!ts) return '—';
            return new Date(ts).toLocaleString();
        },
    };

    function setText(id, value) {
        const el = document.getElementById(id);
        if (el) el.textContent = value;
    }

    function themeColors() {
        const cs = getComputedStyle(root);
        return {
            text:    cs.getPropertyValue('--vt-text').trim()    || '#d6d8dc',
            dim:     cs.getPropertyValue('--vt-text-dim').trim()|| '#8b8e96',
            border:  cs.getPropertyValue('--vt-border').trim()  || '#2f3138',
            accent:  cs.getPropertyValue('--vt-accent').trim()  || '#5a8fd0',
            up:      cs.getPropertyValue('--vt-up').trim()      || '#28c878',
            down:    cs.getPropertyValue('--vt-down').trim()    || '#e55858',
        };
    }

    async function fetchJson(url) {
        try {
            const r = await fetch(url, { cache: 'no-store' });
            if (!r.ok) return null;
            return await r.json();
        } catch (e) {
            return null;
        }
    }

    // ── Dashboard updaters ──────────────────────────────────────────────────
    function renderSnapshot(s) {
        const status = document.getElementById('vt-status');
        if (s && s.connected) {
            if (status) { status.textContent = 'connected'; status.classList.add('connected'); }
        } else {
            if (status) { status.textContent = 'offline'; status.classList.remove('connected'); }
        }

        if (!s) return;
        setText('vt-symbol',  s.symbol || '—');
        setText('vt-price',   fmt.price(s.price));
        const change = document.getElementById('vt-change');
        if (change) {
            change.textContent = fmt.pct(s.change_pct);
            change.classList.remove('up', 'down');
            if (s.change_pct != null) change.classList.add(s.change_pct >= 0 ? 'up' : 'down');
        }
        const a = s.account || {};
        setText('vt-balance',   fmt.money(a.balance));
        setText('vt-equity',    fmt.money(a.equity));
        setText('vt-pnl-open',  fmt.money(a.pnl_open));
        setText('vt-pnl-total', fmt.money(a.pnl_total));
        setText('vt-drawdown',  fmt.pct(a.drawdown != null ? a.drawdown * 100 : null));

        const tbody = document.querySelector('#vt-positions tbody');
        if (tbody) {
            const rows = (s.positions || []);
            if (rows.length === 0) {
                tbody.innerHTML = '<tr><td colspan="6" class="vt-empty">no open positions</td></tr>';
            } else {
                tbody.innerHTML = rows.map(p => {
                    const sideCls = (p.side || '').toUpperCase() === 'BUY' ? 'vt-side-buy' : 'vt-side-sell';
                    const pnlCls = (p.pnl || 0) >= 0 ? 'vt-pnl-pos' : 'vt-pnl-neg';
                    return '<tr>'
                        + '<td>' + (p.symbol || '') + '</td>'
                        + '<td class="' + sideCls + '">' + (p.side || '') + '</td>'
                        + '<td class="num">' + fmt.price(p.qty) + '</td>'
                        + '<td class="num">' + fmt.price(p.entry_price) + '</td>'
                        + '<td class="num">' + fmt.price(p.current_price) + '</td>'
                        + '<td class="num ' + pnlCls + '">' + fmt.money(p.pnl) + '</td>'
                        + '</tr>';
                }).join('');
            }
        }
    }

    function renderTrades(payload) {
        const tbody = document.querySelector('#vt-trades tbody');
        if (!tbody) return;
        const rows = (payload && payload.trades) || [];
        if (rows.length === 0) {
            tbody.innerHTML = '<tr><td colspan="7" class="vt-empty">no trades yet</td></tr>';
            return;
        }
        tbody.innerHTML = rows.map(t => {
            const sideCls = (t.side || '').toUpperCase() === 'BUY' ? 'vt-side-buy' : 'vt-side-sell';
            const pnlCls = (t.pnl || 0) >= 0 ? 'vt-pnl-pos' : 'vt-pnl-neg';
            return '<tr>'
                + '<td>' + fmt.time(t.timestamp) + '</td>'
                + '<td>' + (t.symbol || '') + '</td>'
                + '<td class="' + sideCls + '">' + (t.side || '') + '</td>'
                + '<td class="num">' + fmt.price(t.quantity) + '</td>'
                + '<td class="num">' + fmt.price(t.entry_price || t.entryPrice) + '</td>'
                + '<td class="num">' + fmt.price(t.exit_price  || t.exitPrice)  + '</td>'
                + '<td class="num ' + pnlCls + '">' + fmt.money(t.pnl) + '</td>'
                + '</tr>';
        }).join('');
    }

    // ── Chart ───────────────────────────────────────────────────────────────
    let chart = null;
    function ensureChart() {
        const canvas = document.getElementById('vt-chart');
        if (!canvas || typeof Chart === 'undefined') return null;
        if (chart) return chart;

        const colors = themeColors();
        chart = new Chart(canvas.getContext('2d'), {
            type: 'line',
            data: { labels: [], datasets: [{
                label: 'Close',
                data: [],
                borderColor: colors.accent,
                backgroundColor: colors.accent + '33',
                tension: 0.15,
                pointRadius: 0,
                borderWidth: 1.5,
                fill: true,
            }]},
            options: {
                responsive: true,
                maintainAspectRatio: false,
                animation: false,
                interaction: { intersect: false, mode: 'index' },
                scales: {
                    x: { ticks: { color: colors.dim, maxTicksLimit: 8 },
                         grid: { color: colors.border } },
                    y: { ticks: { color: colors.dim },
                         grid: { color: colors.border } },
                },
                plugins: { legend: { display: false } },
            },
        });
        return chart;
    }

    function applyChartTheme() {
        if (!chart) return;
        const colors = themeColors();
        chart.data.datasets[0].borderColor = colors.accent;
        chart.data.datasets[0].backgroundColor = colors.accent + '33';
        chart.options.scales.x.ticks.color = colors.dim;
        chart.options.scales.x.grid.color  = colors.border;
        chart.options.scales.y.ticks.color = colors.dim;
        chart.options.scales.y.grid.color  = colors.border;
        chart.update('none');
    }
    window.addEventListener('vt:theme-change', applyChartTheme);

    function renderCandles(payload) {
        const c = ensureChart();
        if (!c) return;
        const candles = (payload && payload.candles) || [];
        c.data.labels = candles.map(k => {
            const t = k.openTime || k.open_time || k.t;
            return t ? new Date(t).toLocaleTimeString([], {hour: '2-digit', minute: '2-digit'}) : '';
        });
        c.data.datasets[0].data = candles.map(k => Number(k.close));
        c.update('none');
    }

    // ── Polling ─────────────────────────────────────────────────────────────
    const POLL_MS = window.VT_POLL_MS || 1000;
    async function tick() {
        const tasks = [fetchJson('/api/snapshot.php')];
        if (document.getElementById('vt-chart'))   tasks.push(fetchJson('/api/candles.php'));
        if (document.getElementById('vt-trades'))  tasks.push(fetchJson('/api/trades.php'));
        const [snap, candles, trades] = await Promise.all(tasks);
        if (snap !== undefined)    renderSnapshot(snap);
        if (candles !== undefined) renderCandles(candles);
        if (trades !== undefined)  renderTrades(trades);
    }
    tick();
    setInterval(tick, POLL_MS);
})();
