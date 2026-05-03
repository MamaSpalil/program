<?php
/**
 * Database Configuration for Microsoft SQL Server 2012
 */

return [
    'host' => getenv('DB_HOST') ?: 'localhost',
    'port' => getenv('DB_PORT') ?: '1433',
    'database' => getenv('DB_NAME') ?: 'cryptotrader',
    'username' => getenv('DB_USER') ?: 'sa',
    'password' => getenv('DB_PASS') ?: '',
    'charset' => 'UTF-8',

    // Connection options
    'options' => [
        'ReturnDatesAsStrings' => true,
        'CharacterSet' => 'UTF-8',
        'Encrypt' => true,
        'TrustServerCertificate' => true // For development only
    ],

    // Connection pooling
    'pool' => [
        'enabled' => true,
        'min' => 2,
        'max' => 10
    ]
];
