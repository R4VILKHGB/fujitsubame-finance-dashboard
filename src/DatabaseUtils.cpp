#include "DatabaseUtils.h"
#include <QFile>
#include <QDebug>

bool ensureDatabasePermissions(const QString &filePath) {
    QFile dbFile(filePath);

    // Check current permissions
    QFileDevice::Permissions currentPermissions = dbFile.permissions();
    QFileDevice::Permissions requiredPermissions = QFileDevice::WriteOwner | QFileDevice::ReadOwner;

    // Check if permissions are already valid
    if ((currentPermissions & requiredPermissions) == requiredPermissions) {
        qDebug() << "ensureDatabasePermissions: Permissions are already valid";
        return true;
    }

    // Attempt to set permissions
    if (!dbFile.setPermissions(requiredPermissions)) {
        qWarning() << "ensureDatabasePermissions: Failed to set permissions for database file at:" << filePath;
        return false;
    }

    qDebug() << "Permissions successfully ensured for database file at:" << filePath;
    return true;
}
