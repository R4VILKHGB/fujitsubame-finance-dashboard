#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include "QtSqlIncludes.h"
#include <QObject>
#include <QString>
#include <QSettings>

class SettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString databasePath READ databasePath NOTIFY databasePathChanged)

public:
    explicit SettingsController(QObject *parent = nullptr);

    QString databasePath() const;
    QSqlDatabase getDatabaseConnection();

public slots:
    void setDatabasePath(const QString &path);
    bool connectToDatabase(const QString &path);
    void clearAllData();
    void refreshAllData();

signals:
    void databasePathChanged();
    void dataRefreshed();
    void dataCleared();
    void errorOccurred(const QString &errorMessage);

private:
    QSettings m_settings;
    QString m_databasePath;

    void loadSettings();
};

#endif // SETTINGSCONTROLLER_H
