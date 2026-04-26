<?php

namespace CryptoTrader\Database;

/**
 * Database Connection Manager for Microsoft SQL Server
 */
class Connection
{
    private static $instance = null;
    private $connection = null;
    private $config;

    private function __construct()
    {
        $this->config = require __DIR__ . '/../../config/database.php';
        $this->connect();
    }

    /**
     * Get singleton instance
     */
    public static function getInstance()
    {
        if (self::$instance === null) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    /**
     * Connect to Microsoft SQL Server
     */
    private function connect()
    {
        $serverName = $this->config['host'] . ',' . $this->config['port'];
        $connectionInfo = array_merge([
            'Database' => $this->config['database'],
            'UID' => $this->config['username'],
            'PWD' => $this->config['password']
        ], $this->config['options']);

        $this->connection = sqlsrv_connect($serverName, $connectionInfo);

        if ($this->connection === false) {
            $errors = sqlsrv_errors();
            throw new \Exception('Database connection failed: ' . json_encode($errors));
        }
    }

    /**
     * Get database connection
     */
    public function getConnection()
    {
        // Check if connection is still alive
        if ($this->connection === null || !sqlsrv_ping($this->connection)) {
            $this->connect();
        }
        return $this->connection;
    }

    /**
     * Execute query
     */
    public function query($sql, $params = [])
    {
        $stmt = sqlsrv_query($this->getConnection(), $sql, $params);

        if ($stmt === false) {
            $errors = sqlsrv_errors();
            throw new \Exception('Query execution failed: ' . json_encode($errors));
        }

        return $stmt;
    }

    /**
     * Prepare statement
     */
    public function prepare($sql)
    {
        $stmt = sqlsrv_prepare($this->getConnection(), $sql, []);

        if ($stmt === false) {
            $errors = sqlsrv_errors();
            throw new \Exception('Statement preparation failed: ' . json_encode($errors));
        }

        return $stmt;
    }

    /**
     * Execute prepared statement with parameters
     */
    public function execute($sql, $params = [])
    {
        return $this->query($sql, $params);
    }

    /**
     * Fetch all results
     */
    public function fetchAll($stmt)
    {
        $results = [];
        while ($row = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC)) {
            $results[] = $row;
        }
        return $results;
    }

    /**
     * Fetch single row
     */
    public function fetch($stmt)
    {
        return sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC);
    }

    /**
     * Get last inserted ID
     */
    public function lastInsertId()
    {
        $stmt = $this->query('SELECT SCOPE_IDENTITY() as id');
        $row = $this->fetch($stmt);
        return $row['id'] ?? null;
    }

    /**
     * Begin transaction
     */
    public function beginTransaction()
    {
        return sqlsrv_begin_transaction($this->getConnection());
    }

    /**
     * Commit transaction
     */
    public function commit()
    {
        return sqlsrv_commit($this->getConnection());
    }

    /**
     * Rollback transaction
     */
    public function rollback()
    {
        return sqlsrv_rollback($this->getConnection());
    }

    /**
     * Free statement resources
     */
    public function freeStatement($stmt)
    {
        if ($stmt !== false) {
            sqlsrv_free_stmt($stmt);
        }
    }

    /**
     * Close connection
     */
    public function close()
    {
        if ($this->connection !== null) {
            sqlsrv_close($this->connection);
            $this->connection = null;
        }
    }

    /**
     * Prevent cloning
     */
    private function __clone() {}

    /**
     * Prevent unserialization
     */
    public function __wakeup()
    {
        throw new \Exception('Cannot unserialize singleton');
    }

    /**
     * Close connection on destruct
     */
    public function __destruct()
    {
        $this->close();
    }
}
