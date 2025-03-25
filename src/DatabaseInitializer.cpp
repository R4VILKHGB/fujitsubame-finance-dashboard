#include "DatabaseInitializer.h"
#include "DatabaseUtils.h"
#include "QtSqlIncludes.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>

QString initializeDatabase() {
    // Locate the App Data directory
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        qWarning() << "initializeDatabase: App Data directory not found.";
        return QString();
    }

    // Create the directory if it doesn't exist
    QDir().mkpath(appDataPath);

    //Define the database path
    QString destinationDatabasePath = appDataPath + "/finance_manager.db";

    qDebug() << "initializeDatabase: Initializing database at:" << destinationDatabasePath;

    // Create the SQLite database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "AppConnection");
    db.setDatabaseName(destinationDatabasePath);

    if (!db.open()) { // Explicitly open the database
        // If the file does not exist or cannot be opened
        qCritical() << "initializeDatabase: Failed to open database at:" << destinationDatabasePath
                    << "Error:" << db.lastError().text();
        qCritical() << "Failed to open database after initialization:" << db.lastError().text();
        qCritical() << "Driver available:" << QSqlDatabase::isDriverAvailable("QSQLITE");
        qCritical() << "Available SQLite drivers:" << QSqlDatabase::drivers();
        qCritical() << "Active database connections:" << db.connectionNames();
        qCritical() << "Database names:" << db.databaseName();
        qCritical() << "Database open error:" << db.lastError();
        // return -1; // Exit if the database connection fails
        return QString(); // Return an empty string if opening fails
    }

    // Check if the companies table exists (even if the file exists)
    QSqlQuery checkQuery(db);
    if (!checkQuery.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='companies';")) {
        qCritical() << "initializeDatabase: Failed to check if companies table exists:" << checkQuery.lastError().text();
        db.close();
        return QString();
    }

    if (!checkQuery.next()) {
        // Table does not exist, create it
        QSqlQuery createQuery(db);
        if (!createQuery.exec(R"(
            CREATE TABLE IF NOT EXISTS companies (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL COLLATE NOCASE, -- Case-insensitive comparison
                UNIQUE(name COLLATE NOCASE) -- Enforces case-insensitive uniqueness
            )
        )")) {
            qCritical() << "initializeDatabase: Failed to create the companies table:" << createQuery.lastError().text();
            db.close();
            return QString();
        }

        qDebug() << "initializeDatabase: Companies table created successfully at:" << destinationDatabasePath;
    } else {
        qDebug() << "initializeDatabase: Companies table already exists.";
    }

    if (!checkQuery.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='contracts';")) {
        qCritical() << "initializeDatabase: Failed to check if contracts table exists:" << checkQuery.lastError().text();
        db.close();
        return QString();
    }

    if (!checkQuery.next()) {
        // Table does not exist, create it
        QSqlQuery createQuery(db);
        if (!createQuery.exec(R"(
        CREATE TABLE IF NOT EXISTS contracts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            company_id INTEGER NOT NULL,
            start_month INTEGER NOT NULL,
            start_year INTEGER NOT NULL,
            initial_revenue REAL NOT NULL,
            threshold REAL NOT NULL,
            delay INTEGER NOT NULL,
            second_delay INTEGER DEFAULT 0, -- Field for second delay (multi-pay)
            multi_pay BOOLEAN DEFAULT 0, -- Field to indicate multi-payment option
            percentages TEXT NOT NULL,
            formula TEXT DEFAULT NULL, -- Optional formula field,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (company_id) REFERENCES companies (id) ON DELETE CASCADE
        )
    )")) {
            qCritical() << "initializeDatabase: Failed to create the contracts table:" << createQuery.lastError().text();
            db.close();
            return QString();
        }

        qDebug() << "initializeDatabase: Contracts table created successfully at:" << destinationDatabasePath;
    } else {
        qDebug() << "initializeDatabase: Contracts table already exists.";
    }

    // Ensure correct file permissions
    ensureDatabasePermissions(destinationDatabasePath);

    qDebug() << "initializeDatabase: Database initialized successfully and ready for use at:" << destinationDatabasePath;

    return destinationDatabasePath;
}
