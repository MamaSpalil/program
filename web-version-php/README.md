# VT Web — PHP version

Web-версия дашборда **Crypto ML Trader / VT — Virtual Trade System**.
Lightweight PHP 7.4+/8.x приложение без зависимостей: один встроенный сервер PHP
и статические assets (Chart.js через CDN). Поддерживает тёмную и светлую тему
с переключателем (сохраняется в `localStorage`).

## Структура

```
web-version-php/
├── public/
│   └── index.php          ← entry point (роутер + рендер шаблона)
├── api/
│   ├── snapshot.php       ← REST: текущее состояние (цена, equity, позиции)
│   ├── candles.php        ← REST: свечи для графика
│   └── trades.php         ← REST: история сделок
├── includes/
│   ├── config.php         ← пути к данным от C++ движка
│   ├── data_source.php    ← чтение state.json / candles.json
│   └── render.php         ← общий layout + шапка/футер
├── assets/
│   ├── css/app.css        ← тёмная + светлая темы
│   └── js/app.js          ← переключатель темы, авто-обновление, график
└── README.md
```

## Запуск

```bash
cd web-version-php
# index.php работает и как entry point, и как роутер для php -S
php -S 127.0.0.1:8080 public/index.php
# затем открыть http://127.0.0.1:8080/
```

В production-настройке (Apache/nginx) направьте все несуществующие пути на
`public/index.php`. Каталог `public/` — единственный, который должен быть
доступен веб-серверу.

Зависимости: только PHP 7.4+ (используется `json_decode`, `file_get_contents`).
JavaScript-зависимости подгружаются по CDN (Chart.js v4) — интернет требуется
только при первой загрузке страницы.

## Интеграция с C++ движком

Web-версия читает три JSON-файла, которые должен периодически писать движок
(или экспортировать через отдельный HTTP сервис). По умолчанию пути:

| Файл              | Содержимое                             | Пишется      |
|-------------------|----------------------------------------|--------------|
| `data/state.json` | snapshot цены, equity, открытых позиций| ≤ 1 с        |
| `data/candles.json`| последние ~500 свечей текущего символа | каждый бар   |
| `data/trades.json`| история закрытых сделок                | при закрытии |

Пути можно переопределить через переменные окружения `VT_DATA_DIR` или
правкой `includes/config.php`.

Если файлы отсутствуют — UI работает в demo-режиме с пустым состоянием
(вместо ошибки выводится placeholder).

## Темы

Переключатель в правом верхнем углу. Состояние сохраняется в
`localStorage['vt-theme']`. По умолчанию используется системная тема
(`prefers-color-scheme`).

## Безопасность

- Все API-ответы — `application/json` без кэша.
- Чтение JSON-файлов выполняется через whitelisted-пути из
  `includes/config.php`; пользовательский ввод не передаётся в `file_get_contents`.
- Нет записи на диск, нет SQL — read-only представление.
- В production добавьте reverse proxy (nginx/Caddy) с TLS и ограничением
  доступа по IP/Basic Auth — embed-only, а не публичный сервис.
