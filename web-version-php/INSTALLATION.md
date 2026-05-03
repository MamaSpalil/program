# Installation Guide - CryptoTrader PHP+Java+HTML+CSS

Complete installation guide for CryptoTrader web version using PHP, Java, HTML/CSS, and MSSQL 2012.

## Prerequisites

### System Requirements
- **Operating System:** Windows Server 2012+ or Linux (Ubuntu 20.04+)
- **RAM:** Minimum 4GB, Recommended 8GB+
- **Disk Space:** 10GB+ free space

### Required Software

#### 1. Microsoft SQL Server 2012
- **Download:** https://www.microsoft.com/en-us/sql-server/sql-server-downloads
- **Edition:** Express (free) or higher
- **Installation:**
  - Choose "Basic" installation type
  - Note down the server name (usually `localhost` or `.\SQLEXPRESS`)
  - Set a strong SA password

#### 2. PHP 8.1+
- **Windows:**
  - Download from https://windows.php.net/download/
  - Extract to `C:\php`
  - Add to PATH: `C:\php`
  - Enable extensions in `php.ini`:
    ```ini
    extension=sqlsrv
    extension=pdo_sqlsrv
    extension=mbstring
    extension=openssl
    extension=curl
    ```
  - Download Microsoft SQLSRV drivers: https://docs.microsoft.com/en-us/sql/connect/php/download-drivers-php-sql-server
  - Copy driver DLLs to `C:\php\ext\`

- **Linux:**
  ```bash
  sudo apt-get update
  sudo apt-get install -y php8.1 php8.1-cli php8.1-mbstring php8.1-curl php8.1-xml

  # Install SQLSRV drivers
  curl https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
  curl https://packages.microsoft.com/config/ubuntu/20.04/prod.list > /etc/apt/sources.list.d/mssql-release.list
  sudo apt-get update
  sudo ACCEPT_EULA=Y apt-get install -y msodbcsql17 mssql-tools
  sudo apt-get install -y php8.1-dev php-pear
  sudo pecl install sqlsrv pdo_sqlsrv
  echo "extension=sqlsrv.so" >> /etc/php/8.1/cli/php.ini
  echo "extension=pdo_sqlsrv.so" >> /etc/php/8.1/cli/php.ini
  ```

#### 3. Java 17+
- **Download:** https://adoptium.net/ or https://www.oracle.com/java/technologies/downloads/
- **Installation:**
  - Install JDK 17 or higher
  - Set JAVA_HOME environment variable
  - Add to PATH: `%JAVA_HOME%\bin` (Windows) or `/usr/lib/jvm/java-17/bin` (Linux)
  - Verify: `java -version`

#### 4. Maven 3.6+
- **Download:** https://maven.apache.org/download.cgi
- **Installation:**
  - Extract to `C:\maven` (Windows) or `/opt/maven` (Linux)
  - Add to PATH: `C:\maven\bin` or `/opt/maven/bin`
  - Verify: `mvn -version`

#### 5. Web Server
- **Option A: Apache 2.4+**
  - Windows: https://www.apachelounge.com/download/
  - Linux: `sudo apt-get install apache2 libapache2-mod-php8.1`

- **Option B: Nginx + PHP-FPM**
  - Linux: `sudo apt-get install nginx php8.1-fpm`

#### 6. Composer (PHP Package Manager)
- **Download:** https://getcomposer.org/download/
- **Installation:**
  - Windows: Download and run `Composer-Setup.exe`
  - Linux:
    ```bash
    php -r "copy('https://getcomposer.org/installer', 'composer-setup.php');"
    php composer-setup.php
    sudo mv composer.phar /usr/local/bin/composer
    ```
  - Verify: `composer --version`

## Installation Steps

### Step 1: Clone Repository

```bash
git clone https://github.com/MamaSpalil/program.git
cd program/web-version-php
```

### Step 2: Configure Database

1. **Start SQL Server Management Studio (SSMS)**

2. **Create Database:**
   ```sql
   CREATE DATABASE cryptotrader;
   GO
   ```

3. **Run Schema Script:**
   ```bash
   sqlcmd -S localhost -U sa -P YourPassword -i database/schema.sql
   ```

   Or in SSMS:
   - Open `database/schema.sql`
   - Execute the script (F5)

4. **Verify Installation:**
   ```sql
   USE cryptotrader;
   SELECT * FROM users;
   -- Should show the admin user
   ```

### Step 3: Configure PHP Backend

1. **Install Dependencies:**
   ```bash
   cd backend
   composer install
   ```

2. **Configure Environment:**
   ```bash
   cp .env.example .env
   ```

3. **Edit `.env` file:**
   ```env
   DB_HOST=localhost
   DB_PORT=1433
   DB_NAME=cryptotrader
   DB_USER=sa
   DB_PASS=YourStrongPassword123

   APP_ENV=development
   APP_DEBUG=true
   APP_URL=http://localhost

   ENCRYPTION_KEY=generate-random-32-char-key-here

   ML_SERVICE_URL=http://localhost:8080
   ```

4. **Generate Encryption Key:**
   ```bash
   php -r "echo bin2hex(random_bytes(16));"
   ```
   Copy the output to `ENCRYPTION_KEY` in `.env`

### Step 4: Build Java ML Service

1. **Navigate to ML Service:**
   ```bash
   cd ../ml-service
   ```

2. **Build with Maven:**
   ```bash
   mvn clean package
   ```

   This will:
   - Download all dependencies
   - Compile Java code
   - Run tests
   - Create JAR file in `target/` directory

3. **Verify Build:**
   ```bash
   ls -l target/ml-service-1.0.0.jar
   ```

### Step 5: Configure Web Server

#### Option A: Apache Configuration

1. **Create Virtual Host:**

   **Windows:** Create `C:\Apache24\conf\extra\httpd-vhosts.conf`
   **Linux:** Create `/etc/apache2/sites-available/cryptotrader.conf`

   ```apache
   <VirtualHost *:80>
       ServerName localhost
       DocumentRoot "C:/path/to/program/web-version-php/backend/public"

       <Directory "C:/path/to/program/web-version-php/backend/public">
           AllowOverride All
           Require all granted
           DirectoryIndex index.php
       </Directory>

       Alias /frontend "C:/path/to/program/web-version-php/frontend"
       <Directory "C:/path/to/program/web-version-php/frontend">
           Options Indexes FollowSymLinks
           AllowOverride None
           Require all granted
       </Directory>

       ErrorLog "logs/cryptotrader-error.log"
       CustomLog "logs/cryptotrader-access.log" combined
   </VirtualHost>
   ```

2. **Enable Site (Linux):**
   ```bash
   sudo a2ensite cryptotrader
   sudo a2enmod rewrite
   sudo systemctl restart apache2
   ```

#### Option B: Nginx Configuration

1. **Create Server Block:**

   Create `/etc/nginx/sites-available/cryptotrader`:

   ```nginx
   server {
       listen 80;
       server_name localhost;

       root /path/to/program/web-version-php/backend/public;
       index index.php;

       location / {
           try_files $uri $uri/ /index.php?$query_string;
       }

       location /frontend {
           alias /path/to/program/web-version-php/frontend;
           try_files $uri $uri/ =404;
       }

       location ~ \.php$ {
           fastcgi_pass unix:/run/php/php8.1-fpm.sock;
           fastcgi_index index.php;
           fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
           include fastcgi_params;
       }
   }
   ```

2. **Enable Site:**
   ```bash
   sudo ln -s /etc/nginx/sites-available/cryptotrader /etc/nginx/sites-enabled/
   sudo systemctl restart nginx
   ```

### Step 6: Set File Permissions (Linux)

```bash
cd /path/to/program/web-version-php
sudo chown -R www-data:www-data .
sudo chmod -R 755 .
sudo chmod -R 775 backend/logs backend/cache
```

## Running the Application

### 1. Start Java ML Service

```bash
cd ml-service
java -jar target/ml-service-1.0.0.jar
```

Or run in background:

**Windows:**
```cmd
start /B java -jar target/ml-service-1.0.0.jar
```

**Linux:**
```bash
nohup java -jar target/ml-service-1.0.0.jar > ml-service.log 2>&1 &
```

**Verify ML Service:**
```bash
curl http://localhost:8080/api/ml/health
```

Expected response:
```json
{
  "status": "ok",
  "service": "ML Service",
  "version": "1.0.0",
  "models": {
    "lstm": true,
    "xgboost": true
  }
}
```

### 2. Start Web Server

If using built-in PHP server (development only):
```bash
cd backend/public
php -S localhost:8000
```

If using Apache/Nginx: Already running

### 3. Access Application

Open browser and navigate to:
- **Main App:** http://localhost (or http://localhost:8000 if using PHP built-in server)
- **Login:** Use `admin` / `admin123` (default credentials)

## Testing the Installation

### 1. Test Database Connection

```bash
php -r "require 'backend/src/database/Connection.php'; \$conn = CryptoTrader\Database\Connection::getInstance(); echo 'Connected!';"
```

### 2. Test PHP API

```bash
curl http://localhost/api/health
```

Expected response:
```json
{
  "status": "ok",
  "timestamp": 1234567890,
  "version": "1.0.0"
}
```

### 3. Test ML Service

```bash
curl -X POST http://localhost:8080/api/ml/signal \
  -H "Content-Type: application/json" \
  -d '{"symbol":"BTCUSDT","interval":"15m","candles":[],"indicators":{}}'
```

### 4. Test Login

```bash
curl -X POST http://localhost/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
```

## Troubleshooting

### PHP SQLSRV Extension Not Found
- **Solution:** Ensure SQLSRV drivers are installed and enabled in `php.ini`
- Check: `php -m | grep sqlsrv`

### Cannot Connect to MSSQL
- **Solution:** Check SQL Server is running: `services.msc` (Windows) or `systemctl status mssql-server` (Linux)
- Enable TCP/IP in SQL Server Configuration Manager
- Check firewall allows port 1433

### Java ML Service Fails to Start
- **Solution:** Check Java version: `java -version` (should be 17+)
- Verify Maven build completed successfully
- Check port 8080 is available: `netstat -an | grep 8080`

### Apache/Nginx 403 Forbidden
- **Solution:** Check file permissions
- Verify DocumentRoot path in config
- Check SELinux status (Linux): `sudo setenforce 0` (temporary fix)

### Database Connection Timeout
- **Solution:** Check connection string in `.env`
- Verify SQL Server authentication mode (enable mixed mode)
- Test connection: `sqlcmd -S localhost -U sa -P YourPassword`

## Next Steps

1. **Change Default Passwords:**
   ```sql
   UPDATE users SET password_hash = '$2y$10$...' WHERE username = 'admin';
   ```

2. **Configure Exchange API Keys:**
   - Login to dashboard
   - Navigate to Settings → Exchanges
   - Add your exchange API credentials

3. **Enable HTTPS:**
   - Obtain SSL certificate (Let's Encrypt recommended)
   - Configure web server for HTTPS
   - Update `.env`: `APP_URL=https://yourdomain.com`

4. **Production Optimization:**
   - Set `APP_DEBUG=false` in `.env`
   - Enable PHP OPcache
   - Configure database connection pooling
   - Set up monitoring and logging

## Support

For issues and questions:
- GitHub Issues: https://github.com/MamaSpalil/program/issues
- Documentation: See README.md files in each directory

## Security Notes

⚠️ **IMPORTANT:**
- Change default admin password immediately
- Use strong encryption keys
- Never commit `.env` file
- Enable HTTPS in production
- Regular database backups
- Keep all software updated
