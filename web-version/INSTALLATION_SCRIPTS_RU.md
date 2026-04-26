# CryptoTrader Web Version - Automated Installation Scripts

Эта директория содержит автоматические скрипты для установки и запуска CryptoTrader Web Version на Windows 10 Pro x64.

## 📋 Доступные скрипты

### 🐳 Docker Installation (Рекомендуется)

#### `install-docker.cmd`
Автоматическая установка и запуск с использованием Docker Compose.

**Требования:**
- Docker Desktop установлен и запущен
- Git установлен

**Использование:**
```cmd
install-docker.cmd
```

Скрипт автоматически:
- ✅ Проверяет наличие Docker
- ✅ Создаёт .env файлы из примеров
- ✅ Запускает все сервисы в Docker контейнерах
- ✅ Открывает браузер с приложением

---

### 🔧 Manual Installation (Для разработчиков)

#### `install-manual.cmd`
Настройка окружения для ручной установки.

**Требования:**
- Node.js 20+
- Python 3.11+
- PostgreSQL 15+
- Redis 7+
- Git

**Использование:**
```cmd
install-manual.cmd
```

Скрипт автоматически:
- ✅ Проверяет наличие всех зависимостей
- ✅ Создаёт .env файлы
- ✅ Устанавливает зависимости для backend, frontend и ml-service
- ✅ Создаёт Python виртуальное окружение
- ✅ Запускает миграции базы данных

---

### 🚀 PowerShell Advanced Script

#### `install.ps1`
Продвинутый PowerShell скрипт с расширенными возможностями.

**Использование:**

```powershell
# Docker установка (по умолчанию)
.\install.ps1

# Ручная установка
.\install.ps1 -Mode manual

# Только проверка зависимостей
.\install.ps1 -Mode check

# Без автоматического открытия браузера
.\install.ps1 -SkipBrowser
```

**Возможности:**
- ✅ Автоматическая проверка версий зависимостей
- ✅ Генерация безопасных секретов (JWT_SECRET, ENCRYPTION_KEY)
- ✅ Цветной вывод и детальные логи
- ✅ Проверка здоровья сервисов
- ✅ Поддержка параметров командной строки

---

### ▶️ Start and Stop Scripts

#### `start-all.cmd`
Запускает все сервисы (определяет автоматически Docker или ручной режим).

```cmd
start-all.cmd
```

Для Docker:
- Запускает контейнеры через `docker-compose up -d`

Для ручной установки:
- Открывает 3 терминальных окна для backend, ml-service и frontend

#### `stop-all.cmd`
Останавливает все запущенные сервисы.

```cmd
stop-all.cmd
```

Для Docker:
- Останавливает контейнеры через `docker-compose stop`
- Предлагает удалить контейнеры

Для ручной установки:
- Завершает процессы Node.js и Python

---

## 🎯 Быстрый старт

### Метод 1: Docker (Самый простой)

1. **Установите Docker Desktop:**
   - Скачайте с https://www.docker.com/products/docker-desktop/
   - Установите и запустите
   - Дождитесь, пока иконка Docker станет зелёной

2. **Запустите установку:**
   ```cmd
   install-docker.cmd
   ```

3. **Готово!** Откроется браузер на http://localhost:3000

### Метод 2: Ручная установка

1. **Установите все зависимости:**
   - Node.js 20 LTS: https://nodejs.org/
   - Python 3.11+: https://www.python.org/
   - PostgreSQL 15+: https://www.postgresql.org/
   - Redis 7+: https://redis.io/

2. **Запустите настройку:**
   ```cmd
   install-manual.cmd
   ```

3. **Настройте .env файлы** (особенно `backend/.env`):
   - `DATABASE_URL` - строка подключения к PostgreSQL
   - `REDIS_HOST` - хост Redis
   - `JWT_SECRET` - секретный ключ JWT
   - `ENCRYPTION_KEY` - ключ шифрования (32 символа)

4. **Запустите сервисы:**
   ```cmd
   start-all.cmd
   ```

---

## 🔍 Проверка установки

После запуска проверьте доступность сервисов:

- **Frontend:** http://localhost:3000
- **Backend API:** http://localhost:4000
- **ML Service:** http://localhost:8000/docs

### Для Docker:
```cmd
docker-compose ps
```

Все контейнеры должны быть в статусе "Up".

### Просмотр логов:
```cmd
# Все сервисы
docker-compose logs -f

# Конкретный сервис
docker-compose logs -f backend
```

---

## ⚙️ Управление сервисами

### Docker:

```cmd
# Запустить
docker-compose up -d

# Остановить
docker-compose stop

# Перезапустить
docker-compose restart

# Остановить и удалить контейнеры
docker-compose down

# Пересобрать образы
docker-compose build --no-cache
docker-compose up -d
```

### Ручная установка:

```cmd
# Запустить всё
start-all.cmd

# Остановить всё
stop-all.cmd

# Или запускать каждый сервис отдельно в своём терминале:

# Terminal 1 - Backend
cd backend
npm run dev

# Terminal 2 - ML Service
cd ml-service
venv\Scripts\activate.bat
uvicorn app.main:app --reload --port 8000

# Terminal 3 - Frontend
cd frontend
npm run dev
```

---

## 🛠️ Решение проблем

### "Docker is not running"
- Запустите Docker Desktop из меню Пуск
- Дождитесь зелёной иконки в трее

### "Port already in use"
```cmd
# Найдите процесс на порту (например, 3000)
netstat -ano | findstr :3000

# Завершите процесс (замените PID)
taskkill /PID <PID> /F
```

### "Cannot find module" (Node.js)
```cmd
cd backend  # или frontend
rmdir /s /q node_modules
del package-lock.json
npm install
```

### "ModuleNotFoundError" (Python)
```cmd
cd ml-service
venv\Scripts\activate.bat
pip install -r requirements.txt --force-reinstall
```

### "Database connection failed"
1. Проверьте, что PostgreSQL запущен
2. Проверьте `DATABASE_URL` в `backend/.env`
3. Убедитесь, что база данных `cryptotrader` создана

### Скрипты PowerShell не выполняются
```powershell
# Разрешите выполнение скриптов
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

---

## 🔐 Безопасность

### ⚠️ ВАЖНО:

1. **Для тестирования:**
   - Используйте только testnet биржи
   - Включите режим Paper Trading
   - Не используйте реальные API ключи

2. **Перед продакшеном:**
   - Измените все секреты в `.env` файлах
   - Используйте сложные пароли
   - Включите HTTPS
   - Настройте файрвол

3. **Генерация безопасных секретов:**
   ```powershell
   # JWT_SECRET (64 символа)
   -join ((48..57) + (65..90) + (97..122) | Get-Random -Count 64 | ForEach-Object {[char]$_})

   # ENCRYPTION_KEY (32 символа)
   -join ((48..57) + (65..90) + (97..122) | Get-Random -Count 32 | ForEach-Object {[char]$_})
   ```

---

## 📚 Дополнительные ресурсы

- **Полная инструкция:** [WINDOWS_INSTALLATION_RU.md](WINDOWS_INSTALLATION_RU.md)
- **Документация проекта:** [README.md](README.md)
- **Getting Started:** [GETTING_STARTED.md](GETTING_STARTED.md)

---

## 💡 Советы

1. **Для разработки** используйте ручную установку - она даёт больше контроля
2. **Для тестирования/демо** используйте Docker - это быстрее и проще
3. **Всегда проверяйте логи** при возникновении проблем
4. **Делайте бэкапы** базы данных перед обновлениями
5. **Используйте testnet** для первого знакомства с системой

---

## 📞 Поддержка

При возникновении проблем:

1. Проверьте раздел [Решение проблем](#решение-проблем)
2. Изучите логи сервисов
3. Создайте Issue на GitHub: https://github.com/MamaSpalil/program/issues

---

## 📄 Лицензия

Тот же лицензионный режим, что и у основного проекта CryptoTrader.

---

**Версия:** 1.0
**Последнее обновление:** 26 апреля 2026
