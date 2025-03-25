#include "SettingsController.h"
#include "QtSqlIncludes.h"
#include <QFileDialog>
#include <QUrl>
#include <QRegularExpression>
#include <QSettings>
#include <QDebug>
#include <QObject>
#include <QString>

// Constructor - load initial settings from persistent storage
SettingsController::SettingsController(QObject *parent)
    : QObject(parent), m_settings("Fujitsubame", "FinanceDashboard") {
    loadSettings();
    connectToDatabase(m_databasePath);

    // Ensure QML gets initial path immediately
    emit databasePathChanged();
}

void SettingsController::loadSettings() {
    m_databasePath = m_settings.value("databasePath", "").toString();
    qDebug() << "Loaded database path:" << m_databasePath;
}

QString SettingsController::databasePath() const {
    return m_databasePath;
}

bool SettingsController::connectToDatabase(const QString &path) {
    if (path.isEmpty()) {
        emit errorOccurred("Database path is empty.");
        return false;
    }

    // Remove old connection (if exists)
    {
        QSqlDatabase oldDb = QSqlDatabase::database("AppConnection");
        if (oldDb.isOpen()) {
            oldDb.close();
        }
    }
    QSqlDatabase::removeDatabase("AppConnection");

    // Add new database connection explicitly
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "AppConnection");
    db.setDatabaseName(path);

    if (!db.open()) {
        emit errorOccurred("Failed to open database: " + db.lastError().text());
        qWarning() << "Failed to open database:" << db.lastError().text();
        return false;
    }

    qDebug() << "Database successfully connected at:" << path;

    emit databasePathChanged();
    emit dataRefreshed();  // Notify models to reload explicitly

    return true;
}

void SettingsController::setDatabasePath(const QString &path) {
    QString localPath = QUrl(path).isValid() ? QUrl(path).toLocalFile() : path;

    if (localPath.isEmpty() || !QFile::exists(localPath)) {
        emit errorOccurred("Invalid database file selected.");
        qWarning() << "Invalid database file:" << localPath;
        return;
    }

    if (!connectToDatabase(localPath)) {
        return;
    }

    m_databasePath = localPath;
    m_settings.setValue("databasePath", localPath);

    emit databasePathChanged();
    emit dataRefreshed(); // Models should reload after path change
}

QSqlDatabase SettingsController::getDatabaseConnection() {
    return QSqlDatabase::database("AppConnection");
}

void SettingsController::clearAllData() {
    QSqlDatabase db = getDatabaseConnection();

    if (!db.isOpen()) {
        emit errorOccurred("Database is not connected.");
        return;
    }

    QSqlQuery query(db);
    if (!query.exec("DELETE FROM contracts") || !query.exec("DELETE FROM companies")) {
        emit errorOccurred("Failed to clear data: " + query.lastError().text());
        qWarning() << "Clear data error:" << query.lastError().text();
        return;
    }

    qDebug() << "All data cleared successfully.";
    emit dataCleared();
    emit dataRefreshed(); // Models should update themselves
}

void SettingsController::refreshAllData() {
    qDebug() << "Data refresh triggered.";
    emit dataRefreshed(); // Models should reload their data
}
