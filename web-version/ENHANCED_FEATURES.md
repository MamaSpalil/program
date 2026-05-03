# Новые возможности веб-версии CryptoTrader

## 1. Система приглашений и администрирование

### Регистрация только по приглашению

**Первый пользователь:**
- Автоматически получает роль ADMIN
- Не требует инвайт-код
- Становится главным администратором

**Последующие пользователи:**
- Требуется уникальный код приглашения
- Код привязан к конкретному email
- Код действителен 7 дней (настраивается)
- Одноразовое использование

### API endpoints для администратора

**Генерация приглашения:**
```bash
POST /api/v1/admin/invitations
{
  "email": "user@example.com",
  "expiresInDays": 7
}

Response: {
  "code": "A1B2C3D4E5F6G7H8...",
  "email": "user@example.com",
  "expiresAt": "2026-05-03T00:00:00Z"
}
```

**Просмотр приглашений:**
```bash
GET /api/v1/admin/invitations

Response: [{
  "id": "...",
  "code": "A1B2C3...",
  "email": "user@example.com",
  "expiresAt": "...",
  "usedAt": null,
  "createdAt": "..."
}]
```

**Отзыв приглашения:**
```bash
DELETE /api/v1/admin/invitations/:id
```

**Управление пользователями:**
```bash
# Список пользователей
GET /api/v1/admin/users?status=ACTIVE&role=USER&limit=50

# Изменить роль пользователя
PUT /api/v1/admin/users/:id/role
{ "role": "ADMIN" | "USER" | "DEVELOPER" }

# Изменить статус пользователя
PUT /api/v1/admin/users/:id/status
{ "status": "ACTIVE" | "SUSPENDED" | "DELETED" }

# Статистика системы
GET /api/v1/admin/stats
```

**Регистрация с кодом:**
```bash
POST /api/v1/auth/register
{
  "email": "user@example.com",
  "password": "securePassword123",
  "invitationCode": "A1B2C3D4E5F6G7H8...",
  "firstName": "John",
  "lastName": "Doe"
}
```

## 2. Продвинутый анализ графиков

### Обнаружение зон поддержки/сопротивления

**Алгоритм:**
1. Находит swing highs/lows (локальные экстремумы)
2. Группирует похожие ценовые уровни в зоны
3. Рассчитывает силу зоны (0-1) на основе:
   - Количества касаний
   - Плотности зоны (range)
   - Временного периода

**Получение зон:**
```bash
GET /api/v1/analysis/zones/binance/BTCUSDT/15m?limit=200

Response: {
  "zones": [{
    "type": "SUPPORT" | "RESISTANCE" | "PIVOT",
    "priceLevel": 50000.00,
    "priceTop": 50100.00,
    "priceBottom": 49900.00,
    "strength": 0.85,  // 0-1
    "touches": 5,
    "firstTouch": "2026-04-01T00:00:00Z",
    "lastTouch": "2026-04-25T00:00:00Z"
  }],
  "pivots": {
    "pivot": 50000,
    "r1": 50500,
    "r2": 51000,
    "r3": 51500,
    "s1": 49500,
    "s2": 49000,
    "s3": 48500
  },
  "fibonacci": {
    "high": 52000,
    "low": 48000,
    "level236": 51056,
    "level382": 50472,
    "level500": 50000,
    "level618": 49528,
    "level786": 48856
  },
  "confluences": [{
    "priceLevel": 50000,
    "zones": [...],
    "strength": 0.9
  }],
  "volumeProfile": [{
    "priceLevel": 50000,
    "volume": 1000000,
    "type": "HVN" | "LVN"
  }]
}
```

### Типы зон

**SUPPORT (Поддержка):**
- Ценовой уровень, где покупательское давление останавливает падение
- Swing lows (локальные минимумы)

**RESISTANCE (Сопротивление):**
- Ценовой уровень, где давление продавцов останавливает рост
- Swing highs (локальные максимумы)

**PIVOT (Разворотные точки):**
- Рассчитываются как (High + Low + Close) / 3
- Три метода: standard, fibonacci, camarilla

### Методы расчёта

**1. Pivot Points (3 метода):**
- **Standard:** Классические уровни (R1, R2, R3, S1, S2, S3)
- **Fibonacci:** Используют коэффициенты Фибоначчи (0.382, 0.618)
- **Camarilla:** Более точные внутридневные уровни

**2. Fibonacci Retracement:**
- 23.6%, 38.2%, 50%, 61.8%, 78.6% от диапазона
- Используется для определения потенциальных уровней коррекции

**3. Volume Profile:**
- **HVN (High Volume Node):** Зоны с высоким объёмом (> 150% среднего)
- **LVN (Low Volume Node):** Зоны с низким объёмом (< 50% среднего)
- Указывают на сильные уровни поддержки/сопротивления

**4. Zone Confluence:**
- Обнаружение нескольких зон на похожих ценовых уровнях
- Усиливает значимость уровня

## 3. Сигналы входа и выхода из позиций

### Генерация сигналов

**Получение сигналов:**
```bash
GET /api/v1/analysis/signals/binance/BTCUSDT/15m?limit=200

Response: {
  "entrySignals": [{
    "type": "BUY_ENTRY" | "SELL_ENTRY",
    "price": 50000,
    "timestamp": "2026-04-25T23:00:00Z",
    "reason": "Strong support zone at 49900 (strength: 85%, touches: 5)",
    "confidence": 0.85,
    "stopLoss": 49400,
    "takeProfit": 51200
  }],
  "exitSignals": [{
    "type": "BUY_EXIT" | "SELL_EXIT",
    "price": 51000,
    "timestamp": "2026-04-25T23:30:00Z",
    "reason": "Take profit target reached",
    "confidence": 1.0,
    "stopLoss": 0,
    "takeProfit": 0
  }],
  "totalSignals": 2
}
```

### Типы сигналов

**BUY_ENTRY (Вход в длинную позицию):**
1. **Около поддержки:** Цена в пределах 1% от сильной зоны поддержки
2. **Breakout сопротивления:** Пробой уровня с высоким объёмом

**SELL_ENTRY (Вход в короткую позицию):**
1. **Около сопротивления:** Цена в пределах 1% от сильной зоны сопротивления
2. **Breakdown поддержки:** Пробой вниз с высоким объёмом

**BUY_EXIT / SELL_EXIT (Выход из позиции):**
1. **Stop Loss:** Достигнут уровень стоп-лосса
2. **Take Profit:** Достигнута цель прибыли
3. **Reversal:** Разворот на зоне (отскок от поддержки или отбой от сопротивления)

### Логика размещения Stop Loss и Take Profit

**Для LONG позиций:**
- **Stop Loss:** 1.5% ниже зоны поддержки
- **Take Profit:** Чуть ниже ближайшей зоны сопротивления

**Для SHORT позиций:**
- **Stop Loss:** 1.5% выше зоны сопротивления
- **Take Profit:** Чуть выше ближайшей зоны поддержки

**При breakout:**
- **Stop Loss:** На уровне пробитой зоны (поддержка → сопротивление, vice versa)
- **Take Profit:** Расстояние равное размеру breakout (Risk:Reward 1:1)

### Trailing Stop

**Динамическое обновление стоп-лосса:**
```typescript
// Для LONG: передвигается вверх при росте цены
newStopLoss = currentPrice * (1 - trailingPercent)  // 2% по умолчанию

// Для SHORT: передвигается вниз при падении цены
newStopLoss = currentPrice * (1 + trailingPercent)
```

## 4. Маркеры позиций

### Создание маркера

**API:**
```bash
POST /api/v1/analysis/markers
{
  "exchange": "binance",
  "symbol": "BTCUSDT",
  "interval": "15m",
  "type": "BUY_ENTRY",
  "price": 50000,
  "reason": "Strong support zone",
  "confidence": 0.85,
  "stopLoss": 49400,
  "takeProfit": 51200,
  "zoneId": "zone-uuid" // optional
}
```

**Типы маркеров:**
- `BUY_ENTRY` - Вход в длинную позицию
- `SELL_ENTRY` - Вход в короткую позицию
- `BUY_EXIT` - Выход из короткой позиции
- `SELL_EXIT` - Выход из длинной позиции
- `STOP_LOSS` - Стоп-лосс
- `TAKE_PROFIT` - Тейк-профит

### Просмотр маркеров

```bash
GET /api/v1/analysis/markers/binance/BTCUSDT?interval=15m&type=BUY_ENTRY&executed=false
```

### Отметка исполнения

```bash
PUT /api/v1/analysis/markers/:id/execute
{
  "pnl": 150.50  // Прибыль/убыток
}
```

## 5. Циклическая логика размещения меток

### Автоматическое обнаружение и размещение

**Алгоритм работы:**

1. **Анализ графика:**
   - Загружаются последние 200 свечей
   - Обнаруживаются зоны поддержки/сопротивления
   - Рассчитываются pivot points и Fibonacci

2. **Оценка текущей ситуации:**
   - Определяется текущая цена
   - Находятся ближайшие зоны
   - Проверяется расстояние до зон

3. **Генерация сигналов:**
   - **Если цена около поддержки** → BUY_ENTRY сигнал
   - **Если цена около сопротивления** → SELL_ENTRY сигнал
   - **Если пробой сопротивления** → BUY_ENTRY (breakout)
   - **Если пробой поддержки** → SELL_ENTRY (breakdown)

4. **Для активных позиций:**
   - Проверяется достижение stop loss
   - Проверяется достижение take profit
   - Обнаруживаются развороты на зонах
   - Обновляется trailing stop

5. **Уровень уверенности (Confidence):**
   ```
   confidence = zone_strength × volume_confirmation

   где:
   - zone_strength: 0-1 (сила зоны)
   - volume_confirmation: 1.2 при высоком объёме, 0.8 при нормальном
   ```

### Пример циклической работы

```typescript
// Каждые 15 минут (или по расписанию)
setInterval(async () => {
  // 1. Получить последние свечи
  const candles = await getCandles('binance', 'BTCUSDT', '15m', 200);

  // 2. Обнаружить зоны
  const zones = ChartAnalysis.detectSupportResistanceZones(candles);

  // 3. Генерировать сигналы входа
  const entrySignals = SignalGenerator.generateSignals(candles, zones);

  // 4. Создать маркеры для новых сигналов
  for (const signal of entrySignals) {
    if (signal.confidence > 0.6) {  // Порог уверенности
      await createMarker(signal);
    }
  }

  // 5. Проверить активные позиции
  const activePositions = await getActivePositions();
  const exitSignals = SignalGenerator.generateExitSignals(candles, zones, activePositions);

  // 6. Создать маркеры выхода
  for (const signal of exitSignals) {
    await createMarker(signal);
  }

  // 7. Обновить trailing stops
  for (const position of activePositions) {
    const newStopLoss = SignalGenerator.updateTrailingStop(
      currentPrice,
      position,
      0.02  // 2% trailing
    );
    await updatePosition(position.id, { stopLoss: newStopLoss });
  }
}, 15 * 60 * 1000);  // Каждые 15 минут
```

## 6. База данных

### Новые таблицы

**invitation_codes:**
- Хранит коды приглашений
- Связь с пользователем-создателем
- Связь с пользователем, использовавшим код
- Статус (использован/не использован)
- Дата истечения

**support_resistance_zones:**
- Тип зоны (SUPPORT/RESISTANCE/PIVOT)
- Ценовые уровни (top, bottom, level)
- Сила зоны (0-1)
- Количество касаний
- История (первое/последнее касание)
- Статус (активна/пробита)

**position_markers:**
- Тип маркера (BUY_ENTRY, SELL_ENTRY, etc.)
- Цена и время
- Причина размещения
- Уровень уверенности
- Stop Loss / Take Profit
- Статус исполнения
- PnL (если исполнен)

## 7. Рекомендации по использованию

### Для администратора

1. Сгенерировать инвайт-код для нового пользователя
2. Отправить код и ссылку для регистрации
3. Контролировать использование кодов
4. Управлять ролями и статусами пользователей
5. Просматривать статистику системы

### Для трейдера

1. **Анализ рынка:**
   - Получить зоны поддержки/сопротивления
   - Изучить confluence зоны (самые сильные)
   - Проверить volume profile

2. **Поиск точек входа:**
   - Запросить сигналы для выбранной пары
   - Отфильтровать по уровню confidence (>0.6)
   - Проверить соответствие собственному анализу

3. **Управление позицией:**
   - Создать маркер входа при открытии позиции
   - Установить stop loss и take profit по рекомендациям
   - Использовать trailing stop для защиты прибыли
   - Создать маркер выхода при закрытии

4. **Анализ результатов:**
   - Просмотр исполненных маркеров
   - Анализ PnL по маркерам
   - Оптимизация стратегии

## 8. Безопасность

- ✅ Только администраторы могут генерировать коды
- ✅ Первый пользователь автоматически admin
- ✅ Коды привязаны к email
- ✅ Коды одноразовые
- ✅ Коды имеют срок действия
- ✅ Role-based access control
- ✅ Валидация всех входных данных
- ✅ Audit log для действий администратора

## 9. Следующие шаги

После развёртывания:

1. Создать первого пользователя (станет admin)
2. Настроить параметры анализа (sensitivity, confidence threshold)
3. Подключить биржи
4. Начать сбор исторических данных
5. Запустить циклический анализ
6. Пригласить пользователей через admin panel
