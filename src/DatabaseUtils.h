#ifndef DATABASEUTILS_H
#define DATABASEUTILS_H

#include <QString>

// Function to ensure valid permissions for a database file
bool ensureDatabasePermissions(const QString &filePath);

#endif // DATABASEUTILS_H
