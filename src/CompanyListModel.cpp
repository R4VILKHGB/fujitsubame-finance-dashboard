#include "CompanyListModel.h"
#include "QtSqlIncludes.h"
#include <QDebug>
#include <QRegularExpression>

CompanyListModel::CompanyListModel(QObject *parent)
    : QAbstractListModel(parent) {

    if (!m_db.isValid() || !m_db.isOpen()) {
        qWarning() << "CompanyListModel: Database is not valid or open in CompanyListModel!";
    } else {
        qDebug() << "CompanyListModel: Database connection is valid.";
    }

    // Load initial data from the database
    loadCompanies();

    qDebug() << "CompanyListModel initialized with" << rowCount() << "companies.";
}

/// FILTER MANAGEMENT
// Getter for filterString
QString CompanyListModel::filterString() const {
    return m_filterString;
}

// Setter for filterString
void CompanyListModel::setFilterString(const QString &filter) {
    QString trimmedFilter = filter.trimmed(); // Ensure filter string is always stored in a trimmed form
    if (m_filterString != trimmedFilter) {
        m_filterString = trimmedFilter; // Assign trimmed value, updating the filter string
        updateFilter(); // Reapply the filter whenever the string changes
        emit filterStringChanged(); // Notify listeners
    }
}

// Update filter logic
void CompanyListModel::updateFilter() {
    qDebug() << "updateFilter: Updating filter with string:" << m_filterString;

    beginResetModel(); // Notify the view that the model is being reset
    m_filteredIndices.clear();

    // Handle edge case: no companies in the list
    if (m_companies.isEmpty()) {
        qDebug() << "updateFilter: No companies available to filter.";
        endResetModel(); // Ensure model reset completes even with no data
        return;
    }

    // Determine whether to apply filtering or show all companies
    if (m_filterString.isEmpty()) {
        // No filter: show all companies
        for (int i = 0; i < m_companies.size(); ++i) {
            if (!m_companies[i].name.isEmpty()) { // Skip invalid or empty names
                m_filteredIndices.append(i);
            } else {
                qWarning() << "updateFilter: Skipping company with invalid or empty name at index:" << i;
            }
        }
        qDebug() << "updateFilter: No filter applied. Showing all companies.";
    } else {
        // Apply the filter string (case-sensitive)
        for (int i = 0; i < m_companies.size(); ++i) {
            const Company &company = m_companies[i];
            if (!company.name.isEmpty() && company.name.contains(m_filterString, Qt::CaseInsensitive)) {
                m_filteredIndices.append(i);
            }
        }
        qDebug() << "updateFilter: Filter applied. Filtered indices:" << m_filteredIndices;
    }

    endResetModel(); // Notify the view that the model reset is complete
}

// Row count
int CompanyListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0; // No child rows for a flat list model
    return m_filteredIndices.size(); // Return filtered rows count
    // return m_companies.size();  // Return the number of loaded companies in the model
}

// Provide data for the given role and index
QVariant CompanyListModel::data(const QModelIndex &index, int role) const {
    // Validate index
    if (!index.isValid() || index.row() >= m_filteredIndices.size()){
        if (index.isValid()) { // Only log when the index itself is valid
            qWarning() << "data: Invalid index accessed in CompanyListModel data(): row =" << index.row();
        } else {
            qWarning() << "data: Invalid QModelIndex accessed in CompanyListModel data()";
        }
        return QVariant(); // Invalid index or out of bounds
    }

    // Retrieve the company using the filtered index
    const Company &company = m_companies[m_filteredIndices[index.row()]]; // Uses the filtered index to look up the corresponding cache (m_companies) entry

    // Return data based on the requested role
    switch (role) {
    case IdRole:
        return company.id; // Return the unique ID
    case NameRole:
        return company.name; // Return the company name
    default:
        return QVariant(); // No data for unsupported roles
    }
}

// Map roles to role names for QML
QHash<int, QByteArray> CompanyListModel::roleNames() const {
    return {
        {IdRole, "id"}, // Maps the IdRole to "id", used to expose the company's unique ID
        {NameRole, "name"} // Maps the NameRole to "name", used to display the company's name
    };
}

/// COMPANY LIST MANAGEMENT
// Fetch companies from the database, sort by ID
void CompanyListModel::loadCompanies() {
    // Check database connection
    if (!m_db.isOpen()) {
        qWarning() << "loadCompanies: Database is not open! Cannot load companies.";
        return;
    }

    QSqlQuery query(m_db);
    if (!query.exec("SELECT id, name FROM companies ORDER BY id ASC")) {
        qWarning() << "loadCompanies: Failed to load companies from database:" << query.lastError().text();
        return; // Exit early on query failure
    }

    m_companies.clear();  // Clear any existing data in memory, if query successful

    // Load all companies from the database into the model
    while (query.next()) { // Populate the in-memory list with data from the database
        int id = query.value("id").toInt();
        QString name = query.value("name").toString().trimmed(); // Trim the name

        // Minimal validation
        if (id <= 0 || name.isEmpty()) {
            qWarning() << "loadCompanies: Invalid ID or invalid/ empty company name encountered. Skipping row. ID:" << id << "Name:" << name;
            continue; // Skip invalid rows
        }

        m_companies.append({id, name}); // Add to cache
    }

    qDebug() << "loadCompanies: Loaded companies from database. Count:" << m_companies.size();

    updateFilter(); // Apply the current filter after loading data (notifies the UI)
}

// Add a company to the database and model
bool CompanyListModel::addCompany(const QString &name) {
    // Validate name for non-empty and non-whitespace input
    if (!isNameValid(name)) {
        qWarning() << "addCompany: Invalid name provided:" << name;
        return false;
    }

    // Check for duplicate names (case-insensitive)
    if (isDuplicateName(name)) { // excludeId defaults to -1
        qWarning() << "addCompany: Duplicate name provided:" << name;
        return false;
    }

    QString trimmedName = name.trimmed();

    // Check database connection
    if (!m_db.isOpen()) {
        qWarning() << "addCompany: Database is not open!";
        return false;
    }

    // Add company to the database
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO companies (name) VALUES (:name)");
    query.bindValue(":name", trimmedName); // Only bind the name

    if (!query.exec()) {
        qWarning() << "addCompany: Failed to add company to database:" << query.lastError().text();
        return false;
    }

    // Retrieve the newly inserted company's ID
    int newId = query.lastInsertId().toInt();
    if (newId <= 0) {
        qWarning() << "addCompany: Failed to retrieve the new company's ID.";
        return false;
    }

    // Add to local cache
    m_companies.append({newId, trimmedName});

    // Emit signals to notify listeners
    emit companyAdded(newId, trimmedName); // Notify listeners

    updateFilter(); // Refresh filtered indices and notify the UI

    qDebug() << "addCompany: Successfully added company with ID:" << newId << "Name:" << trimmedName;
    return true;
}

// Edit a company
bool CompanyListModel::editCompany(int id, const QString &newName) {
    // Validate input name
    if (!isNameValid(newName)) {
        qWarning() << "editCompany: Invalid name provided:" << newName;
        return false;
    }

    // Check for duplicate names, excluding the current company ID
    if (isDuplicateName(newName, id)) { // Exclude current company ID
        qWarning() << "editCompany: Duplicate name provided:" << newName;
        return false;
    }

    // Trim the name once and use it consistently
    QString trimmedName = newName.trimmed();

    // // Find the company in the in-memory list (validating its ID)
    auto it = std::find_if(m_companies.begin(), m_companies.end(),
                           [id](const Company &company) { return company.id == id; });
    if (it == m_companies.end()) {
        qWarning() << "editCompany: No company found with ID:" << id;
        return false;
    }

    // Check database connection
    if (!m_db.isOpen()) {
        qWarning() << "editCompany: Database is not open!";
        return false;
    }

    // Update the database
    QSqlQuery query(m_db);
    query.prepare("UPDATE companies SET name = :name WHERE id = :id");
    query.bindValue(":name", trimmedName);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "editCompany: Failed to update company in database:" << query.lastError().text();
        return false;
    }

    // Update in-memory cache
    it->name = trimmedName;

    // Emit signals to notify listeners
    emit companyEdited(id, trimmedName);

    // Refresh filtered indices and notify the UI
    updateFilter();

    qDebug() << "editCompany: Successfully updated company with ID:" << id << "to Name:" << trimmedName;
    return true;
}

// Remove a company from the database and model
bool CompanyListModel::removeCompany(int companyId) {
    // Find the company in the in-memory list (validating its ID)
    auto it = std::find_if(m_companies.begin(), m_companies.end(),
                           [companyId](const Company &company) { return company.id == companyId; });

    if (it == m_companies.end()) {
        qWarning() << "removeCompany: No company found with ID:" << companyId;
        return false;
    }

    // Check database connection
    if (!m_db.isOpen()) {
        qWarning() << "removeCompany: Database is not open!";
        return false;
    }

    // Remove the company from the database
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM companies WHERE id = :id");
    query.bindValue(":id", companyId);

    if (!query.exec()) {
        qWarning() << "removeCompany: Failed to remove company from database:" << query.lastError().text();
        return false;
    }

    // Remove from the cache only after successful query execution
    m_companies.erase(it);

    // Emit signal to notify listeners
    emit companyRemoved(companyId);

    // Refresh the filtered list and notify the UI
    updateFilter();

    qDebug() << "removeCompany: Successfully removed company with ID:" << companyId
             << "and all associated contracts via ON DELETE CASCADE.";
    return true;
}

/// CACHE ACCESS AND MANAGEMENT
// Retrieve individual company ID by name
int CompanyListModel::getCompanyIdByName(const QString &name) const {
    // Validate input
    QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        qWarning() << "getCompanyIdByName: Provided name is empty or whitespace.";
        return -1; // Return -1 for invalid input
    }

    // Handle edge case: no companies in the list
    if (m_companies.isEmpty()) {
        qWarning() << "getCompanyIdByName: No companies available in the list.";
        return -1; // No companies in the cache
    }

    // Search for the company by name (case-insensitive)
    for (const Company &company : m_companies) {
        if (company.name.compare(trimmedName, Qt::CaseInsensitive) == 0) {
            qDebug() << "getCompanyIdByName: Found company ID" << company.id << "for name:" << trimmedName;
            return company.id; // Return immediately upon finding a match
        }
    }

    // No match found
    qWarning() << "getCompanyIdByName: No company found with name:" << trimmedName;
    return -1; // Return -1 if no match
}

//// SETTINGS
void CompanyListModel::reloadFromDatabase() {
    qDebug() << "CompanyListModel::reloadFromDatabase triggered.";

    m_db = QSqlDatabase::database("AppConnection");  // Reacquire the connection in case path changed

    m_companies.clear();
    m_filteredIndices.clear();

    loadCompanies();  // Reload companies from the new database
}

/// VALIDATION AND UTILITY FUNCTIONS
// Company name validation
bool CompanyListModel::isNameValid(const QString &name) const {
    QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        qWarning() << "isNameValid: Name is empty or contains only whitespace.";
        return false;
    }
    return true;
}

// Duplicate name check
bool CompanyListModel::isDuplicateName(const QString &name, int excludeId) const {
    QString trimmedName = name.trimmed();
    // Validate name
    if (trimmedName.isEmpty()) {
        qWarning() << "isDuplicateName: Provided name is empty or whitespace.";
        return false; // Empty names are not duplicates
    }

    // Check in-memory cache
    for (const Company &company : m_companies) {
        if (company.id != excludeId && company.name.compare(trimmedName, Qt::CaseInsensitive) == 0) {
            qDebug() << "isDuplicateName: Duplicate name found in cache:" << trimmedName
                     << "excluding ID:" << excludeId;
            return true;
        }
    }

    // Check the database for duplicates
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT COUNT(*)
        FROM companies
        WHERE name = :name COLLATE NOCASE
          AND id != :excludeId
    )");
    query.bindValue(":name", trimmedName);
    query.bindValue(":excludeId", excludeId);

    if (query.exec() && query.next()) {
        int count = query.value(0).toInt();
        if (count > 0) {
            qDebug() << "isDuplicateName: Duplicate name found in database:" << trimmedName
                     << "excluding ID:" << excludeId;
            return true;
        }
    } else {
        qWarning() << "isDuplicateName: Failed to check duplicates in database:" << query.lastError().text();
    }

    return false; // No duplicates found
}

