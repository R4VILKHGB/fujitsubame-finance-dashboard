#ifndef COMPANYLISTMODEL_H
#define COMPANYLISTMODEL_H

#include "QtSqlIncludes.h"
#include <QAbstractListModel>
#include <QVariantList>
#include <QVariant>
#include <QList>
#include <QString>
#include <QDebug>

class CompanyListModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString filterString READ filterString WRITE setFilterString NOTIFY filterStringChanged)

public:
    explicit CompanyListModel(QObject *parent = nullptr);

    // Role enumeration for model data roles
    enum CompanyRoles {
        IdRole = Qt::UserRole + 1,  // Unique company ID
        NameRole                    // Company name
    };

    // Standard overrides for list models
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Filter management
    QString filterString() const;
    void setFilterString(const QString &filter);

    // Company list management
    Q_INVOKABLE void loadCompanies();                                                            // Load companies from the database
    Q_INVOKABLE bool addCompany(const QString &name);                                            // Add a new company
    Q_INVOKABLE bool editCompany(int id, const QString &newName);                                // Edit an existing company
    Q_INVOKABLE bool removeCompany(int companyId);                                               // Remove a company by ID

    // Cache access and management
    Q_INVOKABLE int getCompanyIdByName(const QString &name) const;                               // Retrieve a company ID by name

signals:
    // Filter and company signals
    void filterStringChanged();                                                         // Emitted when filter changes
    void companyAdded(int id, const QString &name);                                     // Emitted when a company is added
    void companyEdited(int id, const QString &newName);                                 // Emitted when a company is edited
    void companyRemoved(int id);                                                        // Emitted when a company is removed

public slots:
    // Settings
    void reloadFromDatabase();

private:
    // Struct to store company data
    struct Company {
        int id;
        QString name;
    };

    // Validation and utility functions
    void updateFilter();                                                // Update filtered indices based on the filter string
    bool isNameValid(const QString &name) const;
    bool isDuplicateName(const QString &name, int excludeId = -1) const;

    // Internal members
    QList<Company> m_companies;                                         // Internal cache of companies, initialized as empty by Qt
    QList<int> m_filteredIndices;                                       // Indices for filtered view, initialized as empty by Qt
    QString m_filterString = "";                                        // Current filter string (default to empty)
    QSqlDatabase m_db = QSqlDatabase::database("AppConnection");        // Global database connection
};

#endif // COMPANYLISTMODEL_H
