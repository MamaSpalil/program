# Полная инструкция по установке CryptoTrader Web Version на Windows 10 Pro x64

Данное руководство содержит подробные инструкции по установке и настройке веб-версии CryptoTrader на локальном компьютере с операционной системой Windows 10 Pro x64.

## Содержание

1. [Системные требования](#системные-требования)
2. [Предварительная подготовка](#предварительная-подготовка)
3. [Установка необходимого ПО](#установка-необходимого-по)
4. [Метод 1: Быстрая установка с Docker (рекомендуется)](#метод-1-быстрая-установка-с-docker-рекомендуется)
5. [Метод 2: Ручная установка для разработки](#метод-2-ручная-установка-для-разработки)
6. [Первый запуск и проверка](#первый-запуск-и-проверка)
7. [Настройка и конфигурация](#настройка-и-конфигурация)
8. [Решение проблем](#решение-проблем)
9. [Безопасность](#безопасность)

---

## Системные требования

### Минимальные требования:
- **ОС**: Windows 10 Pro x64 (версия 1903 или новее)
- **Процессор**: Intel Core i5 / AMD Ryzen 5 или выше
- **Оперативная память**: 8 ГБ RAM (рекомендуется 16 ГБ)
- **Свободное место на диске**: 10 ГБ
- **Интернет**: Широкополосное подключение

### Рекомендуемые требования:
- **ОС**: Windows 10 Pro x64 (последняя версия)
- **Процессор**: Intel Core i7 / AMD Ryzen 7 или выше
- **Оперативная память**: 16 ГБ RAM или больше
- **Свободное место на диске**: 20 ГБ SSD
- **Интернет**: Стабильное широкополосное подключение

---

## Предварительная подготовка

### 1. Проверка версии Windows

Откройте PowerShell (от имени администратора) и выполните:

```powershell
systeminfo | findstr /B /C:"OS Name" /C:"OS Version"
```

Убедитесь, что у вас Windows 10 Pro x64.

### 2. Включение компонентов Windows

Для Docker Desktop необходимо включить WSL 2 (Windows Subsystem for Linux):

1. Откройте PowerShell от имени администратора
2. Выполните команду:

```powershell
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart
```

3. **Перезагрузите компьютер**

4. После перезагрузки установите WSL 2:

```powershell
wsl --install
wsl --set-default-version 2
```

### 3. Обновление Windows

Убедитесь, что у вас установлены все последние обновления Windows:
- Откройте **Параметры** → **Обновление и безопасность** → **Центр обновления Windows**
- Нажмите **Проверить наличие обновлений**

---

## Установка необходимого ПО

### 1. Git для Windows

Git необходим для клонирования репозитория.

**Установка:**

1. Скачайте Git с официального сайта: https://git-scm.com/download/win
2. Запустите установщик
3. Используйте настройки по умолчанию (рекомендуется)
4. В настройках выберите "Git from the command line and also from 3rd-party software"

**Проверка установки:**

```powershell
git --version
```

Должна отобразиться версия Git (например, `git version 2.43.0.windows.1`).

### 2. Node.js 20 LTS

Node.js требуется для работы backend и frontend частей приложения.

**Установка:**

1. Скачайте Node.js 20 LTS с официального сайта: https://nodejs.org/
2. Скачайте версию **LTS (Long Term Support)** для Windows (x64)
3. Запустите установщик (`node-v20.x.x-x64.msi`)
4. Следуйте инструкциям установщика
5. Убедитесь, что установщик включает npm и добавляет Node.js в PATH

**Проверка установки:**

```powershell
node --version
npm --version
```

Версия Node.js должна быть 20.x.x или новее.
Версия npm должна быть 10.x.x или новее.

### 3. Python 3.11

Python необходим для ML-сервиса (Machine Learning).

**Установка:**

1. Скачайте Python 3.11 с официального сайта: https://www.python.org/downloads/
2. Скачайте Windows installer (64-bit)
3. **ВАЖНО**: При установке обязательно отметьте галочку **"Add Python to PATH"**
4. Выберите "Install Now"

**Проверка установки:**

```powershell
python --version
pip --version
```

Версия Python должна быть 3.11.x или новее.

---

## Метод 1: Быстрая установка с Docker (рекомендуется)

Этот метод наиболее простой и быстрый. Docker автоматически настроит все сервисы в изолированных контейнерах.

### Шаг 1: Установка Docker Desktop

1. Скачайте Docker Desktop для Windows: https://www.docker.com/products/docker-desktop/
2. Запустите установщик `Docker Desktop Installer.exe`
3. Следуйте инструкциям установщика
4. После установки **перезагрузите компьютер**
5. Запустите Docker Desktop из меню Пуск
6. Дождитесь полного запуска Docker (иконка Docker в трее должна быть зелёной)

**Проверка установки:**

```powershell
docker --version
docker-compose --version
```

### Шаг 2: Клонирование репозитория

Откройте PowerShell или Command Prompt и выполните:

```powershell
# Перейдите в директорию, где хотите разместить проект (например, Documents)
cd C:\Users\%USERNAME%\Documents

# Клонируйте репозиторий
git clone https://github.com/MamaSpalil/program.git

# Перейдите в директорию web-version
cd program\web-version
```

### Шаг 3: Запуск с Docker Compose

```powershell
# Убедитесь, что вы находитесь в директории web-version
# Запустите все сервисы
docker-compose up -d
```

При первом запуске Docker скачает все необходимые образы и запустит контейнеры. Это может занять 10-15 минут.

**Проверка запуска:**

```powershell
# Проверьте состояние контейнеров
docker-compose ps
```

Все контейнеры должны иметь статус "Up".

### Шаг 4: Доступ к приложению

После успешного запуска сервисы будут доступны по следующим адресам:

- **Frontend (Веб-интерфейс)**: http://localhost:3000
- **Backend API**: http://localhost:4000
- **ML Service**: http://localhost:8000
- **PostgreSQL**: localhost:5432
- **Redis**: localhost:6379

Откройте браузер и перейдите на http://localhost:3000

### Шаг 5: Остановка и управление

```powershell
# Остановить все сервисы
docker-compose stop

# Запустить снова
docker-compose start

# Полностью остановить и удалить контейнеры (данные в БД сохранятся)
docker-compose down

# Посмотреть логи всех сервисов
docker-compose logs

# Посмотреть логи конкретного сервиса
docker-compose logs backend
docker-compose logs frontend
docker-compose logs ml-service
```

---

## Метод 2: Ручная установка для разработки

Этот метод подходит для разработчиков, которым нужен больший контроль над средой разработки.

### Предварительные требования

Убедитесь, что установлены:
- Git (см. выше)
- Node.js 20+ (см. выше)
- Python 3.11+ (см. выше)
- PostgreSQL 15+
- Redis 7+

### Шаг 1: Установка PostgreSQL

1. Скачайте PostgreSQL 15 или новее: https://www.postgresql.org/download/windows/
2. Запустите установщик
3. При установке запомните пароль для пользователя `postgres`
4. Порт по умолчанию: 5432 (оставьте как есть)
5. Установите компонент pgAdmin 4 для управления БД

**Создание базы данных:**

1. Откройте pgAdmin 4
2. Подключитесь к серверу PostgreSQL (введите пароль)
3. Создайте новую базу данных:
   - Правый клик на "Databases" → "Create" → "Database"
   - Name: `cryptotrader`
   - Owner: `postgres`
   - Нажмите "Save"

Или через командную строку PostgreSQL (SQL Shell - psql):

```sql
CREATE DATABASE cryptotrader;
CREATE USER cryptotrader WITH PASSWORD 'cryptotrader_password';
GRANT ALL PRIVILEGES ON DATABASE cryptotrader TO cryptotrader;
```

### Шаг 2: Установка Redis

**Вариант A: Использование Redis для Windows (упрощённый)**

1. Скачайте Redis для Windows с GitHub: https://github.com/tporadowski/redis/releases
2. Скачайте `Redis-x64-X.X.XX.msi`
3. Установите Redis как Windows Service
4. После установки Redis будет запущен автоматически

**Проверка:**

```powershell
redis-cli ping
```

Должно вернуться: `PONG`

**Вариант B: Использование Docker только для Redis и PostgreSQL**

Если вы не хотите устанавливать PostgreSQL и Redis локально:

```powershell
# Запустите только БД и кэш
docker run -d --name postgres -p 5432:5432 `
  -e POSTGRES_DB=cryptotrader `
  -e POSTGRES_USER=cryptotrader `
  -e POSTGRES_PASSWORD=cryptotrader_password `
  postgres:15-alpine

docker run -d --name redis -p 6379:6379 redis:7-alpine
```

### Шаг 3: Клонирование репозитория

```powershell
cd C:\Users\%USERNAME%\Documents
git clone https://github.com/MamaSpalil/program.git
cd program\web-version
```

### Шаг 4: Настройка Backend

```powershell
# Перейдите в директорию backend
cd backend

# Установите зависимости
npm install

# Создайте файл конфигурации
copy .env.example .env

# Откройте .env в текстовом редакторе и настройте
notepad .env
```

**Настройте файл `.env`:**

```env
# Server
NODE_ENV=development
PORT=4000
API_PREFIX=/api/v1

# Database
DATABASE_URL=postgresql://cryptotrader:cryptotrader_password@localhost:5432/cryptotrader

# Redis
REDIS_HOST=localhost
REDIS_PORT=6379
REDIS_PASSWORD=

# JWT (измените в продакшене!)
JWT_SECRET=my-super-secret-key-for-development
JWT_EXPIRES_IN=7d
JWT_REFRESH_EXPIRES_IN=30d

# Encryption (32 символа, измените в продакшене!)
ENCRYPTION_KEY=12345678901234567890123456789012

# ML Service
ML_SERVICE_URL=http://localhost:8000

# Rate Limiting
RATE_LIMIT_WINDOW_MS=60000
RATE_LIMIT_MAX_REQUESTS=100

# CORS
CORS_ORIGIN=http://localhost:3000

# Logging
LOG_LEVEL=info

# Exchange API (тестовые сети по умолчанию - безопасно)
BINANCE_TESTNET=true
BYBIT_TESTNET=true
```

**Инициализация базы данных:**

```powershell
# Генерация Prisma Client
npx prisma generate

# Запуск миграций базы данных
npx prisma migrate dev

# (Опционально) Заполнение тестовыми данными
npx prisma db seed
```

### Шаг 5: Настройка ML Service (Python)

Откройте новое окно PowerShell:

```powershell
# Перейдите в директорию ml-service
cd C:\Users\%USERNAME%\Documents\program\web-version\ml-service

# Создайте виртуальное окружение Python
python -m venv venv

# Активируйте виртуальное окружение
.\venv\Scripts\Activate.ps1
```

**Примечание**: Если возникает ошибка выполнения скриптов, выполните:

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

Затем снова активируйте окружение:

```powershell
.\venv\Scripts\Activate.ps1

# Установите зависимости (это может занять несколько минут)
pip install -r requirements.txt

# Создайте файл конфигурации (если есть .env.example)
copy .env.example .env
```

### Шаг 6: Настройка Frontend

Откройте третье окно PowerShell:

```powershell
# Перейдите в директорию frontend
cd C:\Users\%USERNAME%\Documents\program\web-version\frontend

# Установите зависимости
npm install

# Создайте файл конфигурации
copy .env.example .env

# Откройте и проверьте настройки
notepad .env
```

**Настройте файл `.env`:**

```env
VITE_API_URL=http://localhost:4000/api/v1
VITE_WS_URL=ws://localhost:4000
```

### Шаг 7: Запуск всех сервисов

Теперь у вас должно быть открыто 3 окна PowerShell:

**Окно 1 - Backend:**

```powershell
cd C:\Users\%USERNAME%\Documents\program\web-version\backend
npm run dev
```

Вы должны увидеть:
```
Server is running on port 4000
Database connected successfully
```

**Окно 2 - ML Service:**

```powershell
cd C:\Users\%USERNAME%\Documents\program\web-version\ml-service
.\venv\Scripts\Activate.ps1
uvicorn app.main:app --reload --port 8000
```

Вы должны увидеть:
```
INFO:     Uvicorn running on http://127.0.0.1:8000
```

**Окно 3 - Frontend:**

```powershell
cd C:\Users\%USERNAME%\Documents\program\web-version\frontend
npm run dev
```

Вы должны увидеть:
```
VITE ready in XXX ms
Local:   http://localhost:3000
```

---

## Первый запуск и проверка

### 1. Откройте браузер

Перейдите на адрес: **http://localhost:3000**

### 2. Регистрация учётной записи

1. Нажмите "Регистрация" или "Sign Up"
2. Заполните форму регистрации:
   - Email
   - Пароль (минимум 8 символов)
   - Имя пользователя
3. Нажмите "Зарегистрироваться"

### 3. Вход в систему

1. Используйте созданные учётные данные для входа
2. После успешного входа вы попадёте на главную панель управления

### 4. Проверка работоспособности

**Проверка Backend API:**

Откройте в браузере или используйте curl/Postman:
- http://localhost:4000/api/v1/health (должен вернуть статус "OK")

**Проверка ML Service:**

- http://localhost:8000/docs (документация FastAPI)
- http://localhost:8000/health (статус сервиса)

### 5. Настройка биржевых ключей API

1. Перейдите в раздел "Настройки" (Settings)
2. Найдите раздел "Exchange API Keys"
3. Добавьте ключи API от биржи (для начала используйте testnet):
   - **Binance Testnet**: https://testnet.binance.vision/
   - **Bybit Testnet**: https://testnet.bybit.com/

**ВАЖНО**: Для начального тестирования используйте только testnet (тестовые сети), где торговля идёт виртуальными средствами!

---

## Настройка и конфигурация

### Настройка режимов торговли

В файле `backend/.env` или через веб-интерфейс:

- **Paper Trading (Бумажная торговля)**: Симуляция торговли без реальных ордеров
- **Live Trading (Реальная торговля)**: Реальные ордера на бирже (используйте осторожно!)

### Настройка индикаторов

Доступные технические индикаторы:
- EMA (Exponential Moving Average)
- RSI (Relative Strength Index)
- ATR (Average True Range)
- MACD (Moving Average Convergence Divergence)
- Bollinger Bands

Настройте параметры индикаторов в веб-интерфейсе.

### Настройка управления рисками

- **Max Risk Per Trade**: Максимальный риск на сделку (по умолчанию 2%)
- **Max Drawdown**: Максимальная просадка портфеля
- **Position Sizing**: Метод расчёта размера позиции (Kelly Criterion, Fixed Fraction)

---

## Решение проблем

### Backend не запускается

**Проблема**: "Error: Cannot find module"

**Решение**:
```powershell
cd backend
rm -r node_modules
rm package-lock.json
npm install
```

**Проблема**: "Database connection failed"

**Решение**:
1. Проверьте, что PostgreSQL запущен
2. Проверьте DATABASE_URL в `.env`
3. Проверьте, что база данных создана

```powershell
# Проверка PostgreSQL
psql -U postgres -l
```

### ML Service не запускается

**Проблема**: "ModuleNotFoundError: No module named 'fastapi'"

**Решение**:
```powershell
cd ml-service
.\venv\Scripts\Activate.ps1
pip install -r requirements.txt --force-reinstall
```

**Проблема**: PyTorch не устанавливается или ошибки с CUDA

**Решение**: Установите CPU-версию PyTorch:
```powershell
pip install torch==2.6.0 --index-url https://download.pytorch.org/whl/cpu
```

### Frontend показывает белую страницу

**Проблема**: Пустая страница в браузере

**Решение**:
1. Откройте консоль разработчика (F12)
2. Проверьте наличие ошибок в консоли
3. Проверьте, что backend запущен на порту 4000
4. Проверьте VITE_API_URL в `frontend/.env`

### Docker не запускается

**Проблема**: "Docker daemon is not running"

**Решение**:
1. Запустите Docker Desktop из меню Пуск
2. Дождитесь полного запуска (иконка Docker стала зелёной)
3. Повторите `docker-compose up -d`

**Проблема**: "error during connect"

**Решение**:
1. Убедитесь, что WSL 2 установлен и включён
2. Перезапустите Docker Desktop
3. Перезагрузите компьютер

### Порты уже заняты

**Проблема**: "Port 3000 is already in use"

**Решение**:
```powershell
# Найдите процесс, использующий порт
netstat -ano | findstr :3000

# Завершите процесс (замените PID на номер из предыдущей команды)
taskkill /PID <PID> /F
```

### Redis не подключается

**Проблема**: "Error: Redis connection failed"

**Решение**:
```powershell
# Проверьте статус Redis
redis-cli ping

# Если Redis не запущен, запустите его
# Для Docker:
docker start redis

# Для Windows Service:
# Откройте Services.msc → найдите Redis → Start
```

---

## Безопасность

### ⚠️ ВАЖНЫЕ РЕКОМЕНДАЦИИ ПО БЕЗОПАСНОСТИ

#### Для разработки (локальный компьютер):

1. **Используйте только testnet**: Никогда не используйте реальные ключи API на этапе тестирования
2. **Paper trading mode**: Начните с режима бумажной торговли
3. **Не публикуйте ключи**: Никогда не коммитьте файлы `.env` в Git

#### Для продакшена (если планируете):

1. **Смените все секреты**:
   - `JWT_SECRET`: Сгенерируйте случайную строку 64+ символа
   - `ENCRYPTION_KEY`: Обязательно 32 символа, случайные
   - Пароли баз данных

2. **Используйте HTTPS**: Настройте SSL/TLS сертификат

3. **Файрвол**: Закройте все порты кроме 443 (HTTPS)

4. **Регулярные обновления**: Обновляйте зависимости
```powershell
npm audit fix
pip list --outdated
```

5. **Мониторинг**: Настройте логирование и оповещения

6. **Бэкапы**: Регулярно создавайте резервные копии базы данных

### Генерация безопасных секретов

```powershell
# Генерация JWT_SECRET (64 символа)
[System.Convert]::ToBase64String((1..48 | ForEach-Object { Get-Random -Minimum 0 -Maximum 256 }))

# Генерация ENCRYPTION_KEY (32 символа)
-join ((48..57) + (65..90) + (97..122) | Get-Random -Count 32 | ForEach-Object {[char]$_})
```

---

## Дополнительные инструменты

### Prisma Studio (управление БД)

Prisma Studio - это графический интерфейс для просмотра и редактирования данных в PostgreSQL.

```powershell
cd backend
npx prisma studio
```

Откроется браузер с интерфейсом на http://localhost:5555

### Мониторинг логов

**Docker:**
```powershell
docker-compose logs -f
docker-compose logs -f backend
docker-compose logs -f frontend
docker-compose logs -f ml-service
```

**Ручная установка:**
Логи отображаются в терминалах, где запущены сервисы.

### Остановка всех сервисов

**Docker:**
```powershell
docker-compose down
```

**Ручная установка:**
Нажмите `Ctrl+C` в каждом окне PowerShell, где запущены сервисы.

---

## Обновление приложения

### Обновление из Git

```powershell
cd C:\Users\%USERNAME%\Documents\program
git pull origin main
```

**Docker:**
```powershell
cd web-version
docker-compose down
docker-compose build
docker-compose up -d
```

**Ручная установка:**
```powershell
# Backend
cd backend
npm install
npx prisma migrate deploy

# ML Service
cd ../ml-service
.\venv\Scripts\Activate.ps1
pip install -r requirements.txt

# Frontend
cd ../frontend
npm install
```

---

## Полезные ссылки

- **GitHub репозиторий**: https://github.com/MamaSpalil/program
- **Node.js**: https://nodejs.org/
- **Python**: https://www.python.org/
- **Docker Desktop**: https://www.docker.com/products/docker-desktop/
- **PostgreSQL**: https://www.postgresql.org/
- **Redis**: https://redis.io/

- **Binance Testnet**: https://testnet.binance.vision/
- **Bybit Testnet**: https://testnet.bybit.com/

---

## Поддержка

Если у вас возникли проблемы, не описанные в этом руководстве:

1. Проверьте раздел [Решение проблем](#решение-проблем)
2. Изучите логи сервисов
3. Создайте Issue на GitHub: https://github.com/MamaSpalil/program/issues

---

## Лицензия

Проект использует ту же лицензию, что и оригинальная C++ версия CryptoTrader.

---

**Версия документа**: 1.0
**Дата создания**: 26 апреля 2026
**Автор**: CryptoTrader Development Team

---

## Чек-лист перед началом торговли

- [ ] Все сервисы запущены и работают
- [ ] База данных создана и мигрирована
- [ ] Учётная запись зарегистрирована
- [ ] API ключи от биржи настроены (TESTNET!)
- [ ] Режим Paper Trading включен
- [ ] Индикаторы настроены
- [ ] Управление рисками настроено
- [ ] Протестирована связь с биржей
- [ ] Изучена документация по стратегиям
- [ ] Сделаны резервные копии конфигурации

**ВАЖНО**: Начинайте только с testnet и paper trading! Реальная торговля требует тщательного тестирования и понимания рисков!
