#include "ContractModel.h"
#include "CompanyListModel.h"
#include "QtSqlIncludes.h"
#include "qstandardpaths.h"
#include "qdir.h"
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QQuickView>
#include <QRegularExpression>

ContractModel::ContractModel(CompanyListModel *companyModel, QSqlDatabase dbConnection, QObject *parent)
    : QObject(parent),
    m_companyModel(companyModel),
    m_db(dbConnection) {

    if (!m_db.isValid() || !m_db.isOpen()) {
        qWarning() << "ContractModel: Database is not valid or open in ContractModel!";
    } else {
        qDebug() << "ContractModel: Database connection in ContractModel is valid.";
    }

    // Connect signals from CompanyListModel to handle company changes
    if (m_companyModel) {
        connect(m_companyModel, &CompanyListModel::companyAdded, this, &ContractModel::handleCompanyAdded);
        connect(m_companyModel, &CompanyListModel::companyEdited, this, &ContractModel::handleCompanyEdited);
        connect(m_companyModel, &CompanyListModel::companyRemoved, this, &ContractModel::handleCompanyRemoved);
    } else {
        qWarning() << "CompanyListModel pointer is null! ContractModel cannot track company changes.";
    }

    // Full cache initialization
    initializeCache();
}

/// CONTRACT MANAGEMENT
// Retrieve the temporary contract that is being edited, exposing to QML
Q_INVOKABLE QVariantMap ContractModel::getTempContract() {
    return tempContractEdit; // Return the current in-memory contract (holds either an existing contract-> if editing, or a new contract-> if creating)
}

Q_INVOKABLE void ContractModel::initializeTempContractEdit(int companyId, int contractId) {
    // Always reinitialize if creating a new contract
    if (contractId != -1 &&
        tempContractEdit.contains("companyId") && tempContractEdit["companyId"].toInt() == companyId &&
        tempContractEdit.contains("contractId") && tempContractEdit["contractId"].toInt() == contractId) {
        qDebug() << "initializeTempContractEdit: Already initialized, skipping.";
        return;
    }

    // Standard initialization
    tempContractEdit = {
        {"contractId", contractId},    // -1 for new contract
        {"companyId", companyId},
        {"startMonth", QVariant()},
        {"startYear", QVariant()},
        {"initialRevenue", QVariant()},
        {"threshold", QVariant()},
        {"delay", QVariant()},
        {"secondDelay", QVariant()},
        {"multiPay", false},
        {"percentages", QString()},
        {"formula", QString()}
    };

    qDebug() << "initializeTempContractEdit: Initialized with companyId:" << companyId << ", contractId:" << contractId;
}

// ContractModel.cpp
void ContractModel::loadContractIntoTemp(int companyId, int contractId) {
    QVariantMap contract = getContractById(companyId, contractId);

    if (contract.isEmpty() || !contract.contains("contractId")) {
        qWarning() << "loadContractIntoTemp: Invalid or missing contract data";
        return;
    }

    contract["companyId"] = companyId;

    tempContractEdit = contract;

    qDebug() << "loadContractIntoTemp: Editing contractId:" << contractId;

    emit formPopulated(contractId);
}

Q_INVOKABLE QVariantMap ContractModel::getContractById(int companyId, int contractId) {
    QVariantMap contractMap;

    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "getContractById: Company not found:" << companyId;
        return contractMap;  // Return empty
    }

    auto &contracts = m_companyDataCache[companyId].contracts;
    auto it = std::find_if(contracts.begin(), contracts.end(),
                           [contractId](const ContractCondition &c) { return c.contractId == contractId; });

    if (it == contracts.end()) {
        qWarning() << "getContractById: Contract not found:" << contractId;
        return contractMap;  // Return empty
    }

    const ContractCondition &contract = *it;

    contractMap["contractId"] = contract.contractId;
    contractMap["startMonth"] = contract.startMonth;
    contractMap["startYear"] = contract.startYear;
    contractMap["initialRevenue"] = contract.initialRevenue;
    contractMap["threshold"] = contract.threshold;
    contractMap["delay"] = contract.delay;
    contractMap["secondDelay"] = contract.secondDelay;
    contractMap["multiPay"] = contract.multiPay;
    contractMap["percentages"] = formatPercentageListToString(contract.percentages);
    contractMap["formula"] = contract.formula;

    return contractMap;
}

// Return the currently active/loaded contract ID (the contract currently being edited in tempContractEdit)
Q_INVOKABLE int ContractModel::getCurrentContractId() const {
    return tempContractEdit.value("contractId", -1).toInt();
}

bool ContractModel::hasFormChanged(const QVariantMap &formData) const {
    const QStringList fieldsToCheck = {
        "startMonth",
        "startYear",
        "initialRevenue",
        "threshold",
        "delay",
        "secondDelay",
        "multiPay",
        "percentages"
    };

    for (const QString &key : fieldsToCheck) {
        if (!tempContractEdit.contains(key)) {
            qWarning() << "hasFormChanged: Key missing in tempContractEdit:" << key;
            return true;  // Treat missing data as changed (defensive fallback)
        }

        QVariant backendValue = tempContractEdit.value(key);
        QVariant uiValue = formData.value(key);

        if (key == "percentages") {
            // Percentages stored as QString (in formData and tempContractEdit)
            const QString backendStr = backendValue.toString().trimmed();
            const QString uiStr = uiValue.toString().trimmed();
            if (backendStr != uiStr) {
                qDebug() << "hasFormChanged: Field '" << key << "' has changed (percentages)";
                return true;
            }
        } else if (key == "startMonth" || key == "startYear" || key == "delay" || key == "secondDelay") {
            // Integer fields
            const int backendInt = backendValue.toInt();
            const int uiInt = uiValue.toInt();
            if (backendInt != uiInt) {
                qDebug() << "hasFormChanged: Field '" << key << "' has changed (int)";
                return true;
            }
        } else if (key == "initialRevenue" || key == "threshold") {
            // Double fields
            const double backendDouble = backendValue.toDouble();
            const double uiDouble = uiValue.toDouble();
            if (!qFuzzyCompare(backendDouble + 1, uiDouble + 1)) {  // Safer float comparison
                qDebug() << "hasFormChanged: Field '" << key << "' has changed (double)";
                return true;
            }
        } else if (key == "multiPay") {
            // Boolean field
            const bool backendBool = backendValue.toBool();
            const bool uiBool = uiValue.toBool();
            if (backendBool != uiBool) {
                qDebug() << "hasFormChanged: Field '" << key << "' has changed (bool)";
                return true;
            }
        } else {
            // Safety fallback for unexpected fields, ideally should not
            if (backendValue != uiValue) {
                qDebug() << "hasFormChanged: Field '" << key << "' has changed (generic)";
                return true;
            }
        }
    }

    qDebug() << "hasFormChanged: No changes detected.";
    return false; // All fields match exactly
}

Q_INVOKABLE bool ContractModel::canProceedWithSave(const QVariantMap &formData) const {
    if (!canSaveContract(formData)) {
        qDebug() << "canProceedWithSave: Basic form completeness failed.";
        return false;
    }

    if (getCurrentContractId() > 0) {  // Editing mode
        if (!hasFormChanged(formData)) {
            qDebug() << "canProceedWithSave: No changes detected, save disabled.";
            return false;
        }
    }

    qDebug() << "canProceedWithSave: All checks passed, ready to save.";
    return true;
}

Q_INVOKABLE bool ContractModel::prepareAndSaveContract(const QVariantMap &formData) {
    m_lastValidationError.clear();

    qDebug() << "prepareAndSaveContract: Validating saving conditions...";

    // Basic completeness check (required fields must be filled)
    if (!canSaveContract(formData)) {
        m_lastValidationError = tr("Please complete all required fields with valid values before saving.");
        qDebug() << "prepareAndSaveContract: Basic validation failed.";
        return false;
    }

    // Detect changes when editing existing contract
    const bool isEditing = getCurrentContractId() > 0;
    qDebug() << "prepareAndSaveContract: isEditing =" << isEditing;

    // Detect changes only if editing
    if (isEditing && !hasFormChanged(formData)) {
        m_lastValidationError = tr("No changes detected. Please modify the contract before saving.");
        return false;
    }

    // Apply current form state to backend tempContractEdit (overwrite current working draft)
    applyFormToTempContract(formData);

    // Deep form validation (business rules, cross-field logic)
    if (!validateCurrentForm()) {
        qWarning() << "prepareAndSaveContract: Validation failed - " << m_lastValidationError;
        return false; // m_lastValidationError already populated by validateCurrentForm()
    }

    saveContract(); // Save to database and update cache (for both new and existing contract, delegating to saveNewContract() or updateExistingContract())

    return true; // Everything passed and saved successfully
}

// Update a single field in the temporary contract data (used for live updates in QML)
Q_INVOKABLE void ContractModel::updateTempContract(QString key, QVariant value) {
    // Allow initial assignment of contractId/companyId, but do not allow overwriting
    if ((key == "contractId" || key == "companyId")) {
        if (!tempContractEdit.contains(key)) {
            tempContractEdit[key] = value;
            qDebug() << "updateTempContract: Initialized identifier field:" << key << "->" << value;
        } else {
            qDebug() << "updateTempContract: Skipped modifying protected field:" << key;
        }
        return;
    }

    // Field existence check only for editable fields (not strict for identifiers)
    if (!tempContractEdit.contains(key)) {
        qWarning() << "updateTempContract: Invalid field:" << key;
        return;
    }

    // Prevent modifying contractId (database-assigned, should not be changed manually)
    // if (key == "contractId") {
    //     qWarning() << "updateTempContract: Modification of contractId is not allowed!";
    //     return;
    // }

    // Preprocess the value before updating, just in case
    int type = value.typeId();

    if (type == QMetaType::QString) {
        // Allow formula to be an empty string, but trim all other text fields
        if (key != "formula") {
            value = value.toString().trimmed();
        }
    }
    else if (type == QMetaType::Double || type == QMetaType::Float) {
        if (value.toDouble() < 0) { // Prevent negative values for fields that shouldn't allow them
            qWarning() << "updateTempContract: Invalid negative value for field:" << key;
            return;
        }
        value = value.toDouble(); // Explicitly ensure float values stay as double
    }
    else if (type == QMetaType::Int) {
        value = value.toInt(); // Ensure integer fields stay as int
    } else if (type == QMetaType::Bool) {
        value = value.toBool(); // Ensure multiPay is properly cast as a bool
    }

    // Prevent unnecessary updates if the value is unchanged
    if (tempContractEdit[key] == value) {
        return;
    }

    // Update the specific field with the new value
    tempContractEdit[key] = value;

    qDebug() << "updateTempContract: Updated field:" << key << " -> " << value;

    // Emit signals to update UI dynamically
    emit formChanged();  // Notify that the form has been updated
}

// Copy form field values (from QML form) into memory (tempContractEdit)
Q_INVOKABLE void ContractModel::applyFormToTempContract(const QVariantMap &formData) {
    // const QStringList ContractModel::allContractFields = {
    //     "startMonth", "startYear", "initialRevenue", "threshold", "delay", "percentages"
    // };
    // for (const QString &key : allContractFields) {
    //     updateTempContract(key, formData.value(key));
    // }
    updateTempContract("startMonth", formData.value("startMonth"));
    updateTempContract("startYear", formData.value("startYear"));
    updateTempContract("initialRevenue", formData.value("initialRevenue"));
    updateTempContract("threshold", formData.value("threshold"));
    updateTempContract("delay", formData.value("delay"));
    updateTempContract("secondDelay", formData.value("secondDelay"));
    updateTempContract("multiPay", formData.value("multiPay"));
    updateTempContract("percentages", formData.value("percentages"));

    // Ensure identity fields are preserved (critical for editing)
    updateTempContract("contractId", formData.value("contractId", -1));
    updateTempContract("companyId", formData.value("companyId", -1));

    // Formula support, defaulting to empty if unused
    // updateTempContract("formula", formData.value("formula", ""));

    qDebug() << "applyFormToTempContract: Applied form data to tempContractEdit:" << tempContractEdit;
}

// Call corresponding save method, depending if it is new or existing contract
Q_INVOKABLE void ContractModel::saveContract() {
    // Ensure temp contract has the minimum fields
    if (!tempContractEdit.contains("companyId") || !tempContractEdit.contains("contractId")) {
        qWarning() << "saveContract: No contract is being edited or created!";
        // m_lastValidationError = "Unexpected error: No contract is selected for saving.";
        return;
    }

    const int companyId = tempContractEdit["companyId"].toInt();
    const int contractId = tempContractEdit.value("contractId", -1).toInt();

    qDebug() << "saveContract: " << (contractId == -1 ? "Adding new contract" : "Editing existing contract")
             << " for Company ID:" << companyId << ", Contract ID:" << contractId;

    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "saveContract: Invalid company ID:" << companyId << ". Company does not exist in cache.";
        // m_lastValidationError = "Unexpected error: No contract is selected for saving.";
        return;
    }

    // Validate percentages again, as a last check
    QString rawPercentages = tempContractEdit["percentages"].toString(); // Get raw string stored in tempContractEdit
    if (!validatePercentageString(rawPercentages)) {
        qWarning() << "saveContract: Invalid percentages detected right before saving!";
        // m_lastValidationError = "Percentages are invalid or do not sum to 100%. Please review the percentages field.";
        return;
    }

    // Convert percentages
    QList<double> percentageList = parsePercentageStringToList(rawPercentages); // Convert String to QList<double>
    QString formattedPercentages = formatPercentageListToString(percentageList); // Format back to String

    // Save formatted percentages back into tempContractEdit
    tempContractEdit["percentages"] = formattedPercentages;

    // Delegate to saving new or updating existing contract
    bool success = false;
    if (contractId == -1) {
        success = saveNewContract();
    } else {
        success = updateExistingContract(contractId);
    }

    if (success) {
        if (contractId == -1)
            emit contractSavedSuccessfully(); // for new
        else
            emit contractUpdatedSuccessfully(); // for update
    } else {
        qWarning() << "saveContract: Saving failed. Check logs. ";
        // m_lastValidationError = "Failed to save contract to the database. Please check the logs.";
    }
}

// Save new contract to the database and update cache
Q_INVOKABLE bool ContractModel::saveNewContract() {
    // Check if the database is open
    if (!m_db.isOpen()) {
        qWarning() << "saveNewContract: Database is not open, cannot save new contract!";
        return false;
    }

    const int companyId = tempContractEdit.value("companyId").toInt();
    const QString formattedPercentages = tempContractEdit.value("percentages").toString(); // Already formatted in saveContract()

    // Last safety check: percentages should NEVER be empty at this stage
    if (formattedPercentages.isEmpty()) {
        qWarning() << "saveNewContract: Percentages should not be empty!";
        return false;
    }

    // Add a new contract to database
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO contracts (company_id, start_month, start_year, initial_revenue, threshold, delay, second_delay, multi_pay, percentages, formula)
        VALUES (:companyId, :startMonth, :startYear, :initialRevenue, :threshold, :delay, :secondDelay, :multiPay, :percentages, :formula)
    )");

    query.bindValue(":companyId", companyId);
    query.bindValue(":startMonth", tempContractEdit.value("startMonth"));
    query.bindValue(":startYear", tempContractEdit.value("startYear"));
    query.bindValue(":initialRevenue", tempContractEdit.value("initialRevenue"));
    query.bindValue(":threshold", tempContractEdit.value("threshold"));
    query.bindValue(":delay", tempContractEdit.value("delay"));
    query.bindValue(":secondDelay", tempContractEdit.value("secondDelay"));
    query.bindValue(":multiPay", tempContractEdit.value("multiPay"));
    query.bindValue(":percentages", formattedPercentages);

    QString formula = tempContractEdit.value("formula", "").toString().trimmed();
    query.bindValue(":formula", formula.isEmpty() ? QVariant() : formula); // Bind NULL, if unused

    if (!query.exec()) {
        qWarning() << "saveNewContract: Failed to insert new contract:" << query.lastError().text();
        return false; // Stop here, if the database operation fails
    }

    // Retrieve the new contract ID (assigned automatically) from the database (required for cache updates)
    const int newContractId = query.lastInsertId().toInt();
    if (newContractId <= 0) {
        qWarning() << "saveNewContract: Failed to retrieve new contract ID!";
        return false; // Prevent cache update, if database update fails
    }

    // Update tempContractEdit with the new ID
    tempContractEdit["contractId"] = newContractId;

    // Convert percentages to QList<double> for cache storage
    QList<double> percentageList = parsePercentageStringToList(formattedPercentages);

    // Update cache
    ContractCondition newCondition{
        newContractId,
        tempContractEdit.value("startMonth").toInt(),
        tempContractEdit.value("startYear").toInt(),
        tempContractEdit.value("initialRevenue").toDouble(),
        tempContractEdit.value("threshold").toDouble(),
        tempContractEdit.value("delay").toInt(),
        tempContractEdit.value("secondDelay").toInt(),
        tempContractEdit.value("multiPay").toBool(),
        percentageList,  // Store as QList<double> in cache, not QString
        formula
    };

    // int previousCacheSize = m_companyDataCache[companyId].contracts.size();
    m_companyDataCache[companyId].contracts.append(newCondition);

    // qDebug() << "saveNewContract: Successfully added contract for company ID:" << companyId
    //          << ". Cache size before:" << previousCacheSize
    //          << ", Cache size after:" << m_companyDataCache[companyId].contracts.size();

    emit contractChanged(companyId);

    qDebug() << "saveNewContract: New contract saved and cached with ID:" << newContractId;
    return true;
}

// Update existing contract in the database and cache
Q_INVOKABLE bool ContractModel::updateExistingContract(int contractId) {
    // Check if the database is open
    if (!m_db.isOpen()) {
        qWarning() << "updateExistingContract: Database is not open, cannot update contract!";
        return false;
    }

    // Validate contract ID
    if (contractId <= 0) {
        qWarning() << "updateExistingContract: Invalid contract ID!";
        return false;
    }

    const int companyId = tempContractEdit.value("companyId").toInt();
    const QString formattedPercentages = tempContractEdit.value("percentages").toString();  // Already formatted in saveContract()

    // Safety check: percentages should NEVER be empty at this stage
    if (formattedPercentages.isEmpty()) {
        qWarning() << "updateExistingContract: Percentages should not be empty!";
        return false;
    }

    // Update the contract in database
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE contracts
        SET start_month = :startMonth,
            start_year = :startYear,
            initial_revenue = :initialRevenue,
            threshold = :threshold,
            delay = :delay,
            second_delay = :secondDelay,
            multi_pay = :multiPay,
            percentages = :percentages,
            formula = :formula
        WHERE company_id = :companyId AND id = :contractId
    )");

    query.bindValue(":contractId", contractId);
    query.bindValue(":companyId", companyId);  // Extra safety check
    query.bindValue(":startMonth", tempContractEdit.value("startMonth"));
    query.bindValue(":startYear", tempContractEdit.value("startYear"));
    query.bindValue(":initialRevenue", tempContractEdit.value("initialRevenue"));
    query.bindValue(":threshold", tempContractEdit.value("threshold"));
    query.bindValue(":delay", tempContractEdit.value("delay"));
    query.bindValue(":secondDelay", tempContractEdit.value("secondDelay"));
    query.bindValue(":multiPay", tempContractEdit.value("multiPay"));
    query.bindValue(":percentages", formattedPercentages);

    QString formula = tempContractEdit.value("formula", "").toString().trimmed();
    query.bindValue(":formula", formula.isEmpty() ? QVariant() : formula);  // Bind NULL if empty

    if (!query.exec()) {
        qWarning() << "updateExistingContract: Failed to update contract:" << query.lastError().text();
        return false;
    }

    // Update cache (convert percentages back to QList<double>)
    QList<double> percentageList = parsePercentageStringToList(formattedPercentages);

    // Find the contract in cache and update it
    auto &contracts = m_companyDataCache[companyId].contracts;
    bool found = false;  // Track if the contract was updated

    for (ContractCondition &contract : contracts) {
        if (contract.contractId == contractId) {
            // Update contract fields directly
            contract.startMonth = tempContractEdit.value("startMonth").toInt();
            contract.startYear = tempContractEdit.value("startYear").toInt();
            contract.initialRevenue = tempContractEdit.value("initialRevenue").toDouble();
            contract.threshold = tempContractEdit.value("threshold").toDouble();
            contract.delay = tempContractEdit.value("delay").toInt();
            contract.secondDelay = tempContractEdit.value("secondDelay").toInt();
            contract.multiPay = tempContractEdit.value("multiPay").toBool();
            contract.percentages = percentageList;  // Store as QList<double>
            contract.formula = formula;

            qDebug() << "updateExistingContract: Updated contract in cache (ID:" << contractId << ")";
            found = true;
            break;  // Stop loop once contract is found
        }
    }

    // Log a warning if contract was not found in cache
    if (!found) {
        qWarning() << "updateExistingContract: Contract ID" << contractId << "not found in cache for Company ID:" << companyId;
    }

    emit contractChanged(companyId);

    qDebug() << "updateExistingContract: Successfully updated contract ID:" << contractId << " for company ID:" << companyId;
    return true;
}

// Remove a contract
Q_INVOKABLE void ContractModel::removeContract(int companyId, int contractId) {
    // Check if the company ID exists in the cache
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "removeContract: No data found for company ID:" << companyId;
        return;
    }

    // Find the contract in cache using contractId
    auto &contracts = m_companyDataCache[companyId].contracts;
    auto it = std::find_if(contracts.begin(), contracts.end(),
                           [contractId](const ContractCondition &c) { return c.contractId == contractId; });

    if (it == contracts.end()) {
        qWarning() << "removeContract: Contract ID" << contractId << "not found for company ID:" << companyId;
        return;
    }

    // Remove from database first
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM contracts WHERE company_id = :companyId AND id = :contractId");
    query.bindValue(":companyId", companyId);
    query.bindValue(":contractId", contractId);

    bool success = query.exec(); // Execute the query and capture success
    if (!success) {
        qWarning() << "removeContract: Failed to remove contract from database:" << query.lastError().text()
        << "Query:" << query.lastQuery();
        return; // Stop if database operation fails
    }

    qDebug() << "removeContract: Successfully removed contract from database. Company ID:" << companyId
             << ", Contract ID:" << contractId;

    // Remove from cache
    int previousCacheSize = contracts.size();
    contracts.erase(it);

    // Emit signal to notify listeners
    emit contractChanged(companyId);

    qDebug() << "removeContract: Removed contract with ID:" << contractId << "for company ID:" << companyId
             << ". Cache size before:" << previousCacheSize
             << ", Cache size after:" << contracts.size();
}

// Reset form when the user clicks reset button
Q_INVOKABLE void ContractModel::resetFormForManualReset() {
    qDebug() << "resetFormForManualReset: Manual form reset triggered.";

    initializeTempContractEdit(selectedCompanyId(), -1); // Clear any lingering backend data
    // clearSelectedContracts();

    emit formReset(); // Notify QML to reset its UI fields (formState)
}

// Reset form when the users changes company selection
Q_INVOKABLE void ContractModel::resetFormForCompanyChange() {
    qDebug() << "resetFormForCompanyChange: Form reset due to company change.";

    initializeTempContractEdit(selectedCompanyId(), -1); // Clear form data if switching companies
    // clearSelectedContracts(); // Clear selection(s)

    emit formReset(); // Notify QML
}

// Reset form after user saves a contract
// Q_INVOKABLE void ContractModel::resetFormForNewContractAfterSave(bool afterRevenue) {
//     qDebug() << "resetFormForNewContractAfterSave: Reset after successful save (new contract preparation).";

//     // If just saved and are generating revenue, DO NOT reset the form immediately
//     if (afterRevenue) {
//         qDebug() << "resetFormForNewContractAfterSave: Skipping reset because revenue generation is in progress.";
//         return;
//     }

//     initializeTempContractEdit(selectedCompanyId(), -1); // Clear any residual data (new contract flow)

//     emit formReset(); // Notify QML
// }

/// DATABASE AND CACHE MANAGEMENT
// Initialize the whole cache
void ContractModel::initializeCache() {
    if (!m_db.isOpen()) {
        qWarning() << "initializeCache: Database is not open!";
        return;
    }

    qDebug() << "initializeCache: Starting full cache initialization...";

    // Reset persistent internal state
    m_companyDataCache.clear();
    m_revenueData.clear();
    m_revenueSummary.clear();
    m_lastValidationError.clear();
    m_selectedCompanyId = -1;
    m_selectedContractIds.clear();
    tempContractEdit.clear();

    qDebug() << "initializeCache: Cleared internal state and caches.";

    // Load from database
    loadCompanyData();
    loadContracts();

    // Generate revenue per company (contract-based)
    for (const auto &companyId : m_companyDataCache.keys()) {
        loadRevenue(companyId);
    }

    // UI updates
    emit selectedCompanyIdChanged();
    emit selectedContractChanged();
    emit companyCacheUpdated(); // After loadCompanyData
    emit contractsReloaded(); // After loadContracts
    emit formReset();

    qDebug() << "initializeCache: Finished. Companies loaded:" << m_companyDataCache.size();
}

// Fetch company data from database and update cache
void ContractModel::loadCompanyData() {
    // Check if the database is open
    if (!m_db.isOpen()) {
        qWarning() << "loadCompanyData: Database is not open!";
        return;
    }

    qDebug() << "loadCompanyData: Updating company data without clearing contracts."; // m_companyDataCache.clear(); // Clear the existing cache

    // Fetch company data
    QSqlQuery query(m_db);
    if (!query.exec("SELECT id, name FROM companies")) {
        qWarning() << "loadCompanyData: Failed to fetch companies from database:" << query.lastError().text();
        return;
    }

    // Not clearing cache, update the company names
    QHash<int, QString> updatedCompanies;

    while (query.next()) {
        int companyId = query.value("id").toInt();
        QString companyName = query.value("name").toString();
        updatedCompanies[companyId] = companyName;

        // If company exists in cache, update only the name (preserve contracts)
        if (m_companyDataCache.contains(companyId)) {
            m_companyDataCache[companyId].name = companyName;
        } else {
            // If new company, add it with empty contracts
            m_companyDataCache[companyId] = {companyId, companyName}; // Default contracts = {}
        }
    }

    // Remove companies from cache if they no longer exist in the database
    for (auto it = m_companyDataCache.begin(); it != m_companyDataCache.end();) {
        if (!updatedCompanies.contains(it.key())) {
            qDebug() << "loadCompanyData: Removing stale company ID:" << it.key();
            it = m_companyDataCache.erase(it); // Remove company no longer in database
        } else {
            ++it;
        }
    }

    qDebug() << "loadCompanyData: Loaded companies. Cache size:" << m_companyDataCache.size();

    // emit companyCacheUpdated(); // Notify UI components
}

// Fetch all contracts for all companies from database and update cache
void ContractModel::loadContracts() {
    // Check if the database is open
    if (!m_db.isOpen()) {
        qWarning() << "loadContracts: Database is not open!";
        return;
    }

    // Clear only the contract data in the cache (companies remain)
    for (auto &companyData : m_companyDataCache) {
        companyData.contracts.clear();
    }
    qDebug() << "loadContracts: Cleared existing contract data in the cache.";

    // Query contracts from the database
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, company_id, start_month, start_year, initial_revenue, threshold, delay, second_delay, multi_pay, percentages, formula
        FROM contracts
        ORDER BY id ASC
    )");

    if (!query.exec()) {
        qWarning() << "loadContracts: Failed to load contracts from database:" << query.lastError().text();
        return;
    }

    int totalContractsLoaded = 0; // Counter for total contracts

    while (query.next()) {
        int contractId = query.value("id").toInt();
        int companyId = query.value("company_id").toInt();

        // Skip contracts for non-existent companies
        if (!m_companyDataCache.contains(companyId)) {
            qWarning() << "loadContracts: Contract ID:" << contractId << "found for non-existent company ID:" << companyId << ". Skipping.";
            continue;
        }

        // Parse percentages using the helper method
        QList<double> percentages = parsePercentageStringToList(query.value("percentages").toString());
        if (percentages.isEmpty()) {
            qWarning() << "loadContracts: Invalid percentages for Contract ID:" << contractId << ". Skipping.";
            continue;
        }

        // Validate other fields before loading into cache
        int startMonth = query.value("start_month").toInt();
        int startYear = query.value("start_year").toInt();
        double initialRevenue = query.value("initial_revenue").toDouble();
        double threshold = query.value("threshold").toDouble();
        int delay = query.value("delay").toInt();
        int secondDelay = query.value("second_delay").toInt();
        bool multiPay = query.value("multi_pay").toBool();
        QString formula = query.value("formula").toString();

        if (contractId <= 0 || threshold < 0 || delay < 0 || secondDelay < 0 ||
            startMonth < 1 || startMonth > 12 || startYear <= 0 || initialRevenue < 0) {
            qWarning() << "loadContracts: Invalid contract data for Contract ID:" << contractId
                       << " (Threshold:" << threshold
                       << ", Delay:" << delay
                       << ", Second Delay:" << secondDelay
                       << ", Start Month:" << startMonth
                       << ", Start Year:" << startYear
                       << ", Initial Revenue:" << initialRevenue
                       << ", MultiPay:" << multiPay << "). Skipping.";
            continue;
        }

        // Populate contract details
        ContractCondition condition{
            contractId, startMonth, startYear, initialRevenue, threshold, delay, secondDelay, multiPay, percentages, formula
        };

        // Add contract to the company's contract list
        m_companyDataCache[companyId].contracts.append(condition);
        totalContractsLoaded++;

        // Log
        qDebug() << "loadContracts: Loaded Contract ID:" << contractId
                 << "Company ID:" << companyId
                 << "Start Month:" << condition.startMonth
                 << "Start Year:" << condition.startYear
                 << "Initial Revenue:" << condition.initialRevenue
                 << "Threshold:" << condition.threshold
                 << "Delay:" << condition.delay
                 << "Second Delay:" << condition.secondDelay
                 << "MultiPay:" << condition.multiPay
                 << "Formula:" << condition.formula;
    }

    qDebug() << "loadContracts: Loaded" << totalContractsLoaded << "contracts. Updated cache contains contracts for company IDs:"
             << m_companyDataCache.keys();

    // emit contractsReloaded(); // Notify listeners of the reload
}

// Fetch all contracts for a single company from database and update cache
Q_INVOKABLE void ContractModel::loadContractsForCompany(int companyId) {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "loadContractsForCompany: Company ID" << companyId << "does not exist in cache!";
        return;
    }

    // Clear only this company's contracts from cache
    m_companyDataCache[companyId].contracts.clear();

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, start_month, start_year, initial_revenue,
               threshold, delay, second_delay, multi_pay, percentages, formula
        FROM contracts
        WHERE company_id = :companyId
        ORDER BY id ASC
    )");
    query.bindValue(":companyId", companyId);

    if (!query.exec()) {
        qWarning() << "loadContractsForCompany: Failed to load contracts for company ID:" << companyId
                   << "Error:" << query.lastError().text();
        return;
    }

    int contractCount = 0;

    while (query.next()) {
        int contractId = query.value("id").toInt();

        QList<double> percentages = parsePercentageStringToList(query.value("percentages").toString());
        if (percentages.isEmpty()) {
            qWarning() << "loadContractsForCompany: Invalid percentages for Contract ID:" << contractId << ". Skipping.";
            continue;
        }

        int startMonth = query.value("start_month").toInt();
        int startYear = query.value("start_year").toInt();
        double initialRevenue = query.value("initial_revenue").toDouble();
        double threshold = query.value("threshold").toDouble();
        int delay = query.value("delay").toInt();
        int secondDelay = query.value("second_delay").toInt();
        bool multiPay = query.value("multi_pay").toBool();
        QString formula = query.value("formula").toString();

        if (contractId <= 0 || threshold < 0 || delay < 0 || secondDelay < 0 ||
            startMonth < 1 || startMonth > 12 || startYear <= 0 || initialRevenue < 0) {
            qWarning() << "loadContractsForCompany: Skipping invalid contract data for Contract ID:" << contractId;
            continue;
        }

        // Populate contract details
        ContractCondition condition{
            contractId,
            startMonth,
            startYear,
            initialRevenue,
            threshold,
            delay,
            secondDelay,
            multiPay,
            percentages,
            formula
        };

        m_companyDataCache[companyId].contracts.append(condition);
        contractCount++;
    }

    qDebug() << "loadContractsForCompany: Loaded" << contractCount << "contracts for company ID:" << companyId;

    emit contractsReloaded();
}

// Load revenue breakdown for all contracts of a company
// This should auto-refresh all contracts under the company.
// Ensure revenue is always fresh after contract deletions, updates, etc.
Q_INVOKABLE QVariantList ContractModel::loadRevenue(int companyId) {
    QVariantList allRevenue;

    // Ensure company exists
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "loadRevenue: Company ID not found in cache:" << companyId;
        return allRevenue;
    }

    // Ensure revenue data exists
    if (!m_revenueData.contains(companyId)) {
        qWarning() << "loadRevenue: No revenue data found for Company ID:" << companyId << ". Generating...";

        // Auto-generate revenue for all contracts
        for (const auto &contract : m_companyDataCache[companyId].contracts) {
            generateRevenue(companyId, contract.contractId);
        }
    }

    // Iterate through contracts and append revenue breakdowns
    for (const auto &contract : m_companyDataCache[companyId].contracts) {
        allRevenue.append(getRevenueAsVariantList(companyId, contract.contractId));
    }

    qDebug() << "loadRevenue: Loaded revenue for all contracts of Company ID:" << companyId;

    // Compute aggregate revenue metrics
    updateRevenueSummaryCache();

    return allRevenue;
}

// Load revenue breakdown for a single contract of a company
Q_INVOKABLE QVariantList ContractModel::loadRevenueForContract(int companyId, int contractId) {
    QVariantList revenueList;

    // Ensure company exists
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "loadRevenueForContract: Company ID not found in cache:" << companyId;
        return revenueList;
    }

    // Ensure revenue data exists
    if (!m_revenueData.contains(companyId) || !m_revenueData[companyId].contains(contractId)) {
        qWarning() << "loadRevenueForContract: No revenue data found for Contract ID:" << contractId
                   << ". Generating revenue...";
        generateRevenue(companyId, contractId);
    }

    if (!m_revenueData[companyId].contains(contractId)) {
        qWarning() << "loadRevenueForContract: Revenue still not found after generation for Contract ID:" << contractId;
        return revenueList;
    }

    revenueList = getRevenueAsVariantList(companyId, contractId);
    qDebug() << "loadRevenueForContract: Loaded revenue for Contract ID:" << contractId;

    return revenueList;
}

/// DATA RETRIEVAL (USING CACHE)
// Fetch all company data
QMap<int, CompanyData> ContractModel::getAllCompanyData() const {
    return m_companyDataCache; // Return the entire cache
}

// Retrieve an individual company's data by its ID
CompanyData ContractModel::getCompanyDataById(int companyId) const {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "getCompanyDataById: No company found with ID:" << companyId;
        return {}; // Return an empty CompanyData object if ID is invalid
    }
    return m_companyDataCache[companyId];
}

// Fetch all company IDs from cache (sorted already)
Q_INVOKABLE QList<int> ContractModel::getCompanyIds() const {
    QList<int> ids = m_companyDataCache.keys(); // Retrieve all keys (company IDs)

    // Check if the cache is empty
    if (ids.isEmpty()) {
        qWarning() << "getCompanyIds: No companies found in cache.";
        return {};
    }

    std::sort(ids.begin(), ids.end()); // Ensure consistent ordering

    qDebug() << "getCompanyIds: Returning" << ids.size() << "company IDs:" << ids;
    return ids;
}

// Fetch all company names from cache
Q_INVOKABLE QStringList ContractModel::getCompanyNamesList() const {
    QStringList companyNames;

    for (const auto &companyData : m_companyDataCache) { // Loop through the cached data
        companyNames.append(companyData.name); // Append each company name
    }

    qDebug() << "getCompanyNamesList: Returning" << companyNames.size() << "companies";
    return companyNames;
}

// Retrieve individual company ID by name from cache
Q_INVOKABLE int ContractModel::getCompanyIdByName(const QString &name) const {
    // Validate input
    if (!isNameValid(name)) {
        return -1; // Name is invalid (empty/whitespace)
    }

    QString trimmedName = name.trimmed();

    // Use a single pass lookup to find the company ID
    int matchedId = -1;
    bool found = false;

    for (const auto &[companyId, companyData] : m_companyDataCache.asKeyValueRange()) {
        if (companyData.name.compare(trimmedName, Qt::CaseInsensitive) == 0) {
            if (found) { // If a second match is found, return -1 (ambiguous)
                qWarning() << "getCompanyIdByName: Multiple companies found with the name:" << trimmedName;
                return -1; // Ambiguous result
            }
            matchedId = companyId;
            found = true;
        }
    }

    if (!found) {
        qWarning() << "getCompanyIdByName: No company found with the name:" << trimmedName;
        return -1;
    }

    qDebug() << "getCompanyIdByName: Found company ID" << matchedId << "for name:" << trimmedName;
    return matchedId;
}

// Retrieve individual company name based on its ID from cache
Q_INVOKABLE QString ContractModel::getCompanyNameById(int companyId) const {
    auto it = m_companyDataCache.find(companyId);
    if (it == m_companyDataCache.end()) {
        qWarning() << "getCompanyNameById: No company found with ID:" << companyId;
        return QString(); // Return empty string if not found
    }
    return it->name; // Return company name directly
}

// Fetch all contracts for an individual company
Q_INVOKABLE QList<ContractCondition> ContractModel::getContractsByCompanyId(int companyId) const {
    // Find the company in the cache
    auto it = m_companyDataCache.find(companyId);
    if (it == m_companyDataCache.end()) {
        qWarning() << "getContractsByCompanyId: No company found with ID:" << companyId;
        return {}; // Return an empty list if company does not exist
    }

    // Return contracts sorted by contractId
    QList<ContractCondition> sortedContracts = it->contracts;
    std::sort(sortedContracts.begin(), sortedContracts.end(),
              [](const ContractCondition &a, const ContractCondition &b) {
                  return a.contractId < b.contractId;
              });

    return sortedContracts;
}

// Get the contract list for an individual company and convert it into QVariantList for QML compatibility for UI
Q_INVOKABLE QVariantList ContractModel::getContractsAsVariantListByCompanyId(int companyId) const {
    QVariantList contractList;

    // Check if the company exists in the cache
    auto it = m_companyDataCache.find(companyId);
    if (it == m_companyDataCache.end()) {
        qWarning() << "getContractsAsVariantListByCompanyId: No company found with ID:" << companyId;
        return contractList; // Return an empty list
    }

    // Sort contracts before converting to QVariantList
    QList<ContractCondition> sortedContracts = it->contracts;
    std::sort(sortedContracts.begin(), sortedContracts.end(),
              [](const ContractCondition &a, const ContractCondition &b) {
                  return a.contractId < b.contractId;
              });

    // Iterate through contracts and convert to QVariantMap
    for (const ContractCondition &contract : sortedContracts) {
        QVariantMap contractMap{
            { "contractId", contract.contractId },
            { "startMonth", contract.startMonth },
            { "startYear", contract.startYear },
            { "initialRevenue", contract.initialRevenue },
            { "threshold", contract.threshold },
            { "delay", contract.delay },
            { "secondDelay", contract.secondDelay },
            { "multiPay", contract.multiPay },
            { "percentages", formatPercentageListToString(contract.percentages) },
            { "formula", contract.formula }
        };
        contractList.append(contractMap);
    }

    qDebug() << "getContractsAsVariantListByCompanyId: Returning" << contractList.size()
             << "contracts for company ID:" << companyId;

    return contractList;
}

/// STATE VARIABLE MANAGEMENT
Q_INVOKABLE int ContractModel::selectedCompanyId() const {
    return m_selectedCompanyId;
}

Q_INVOKABLE void ContractModel::setSelectedCompanyId(int id) {
    if (m_selectedCompanyId == id) {
        return;  // No change, avoid unnecessary updates
    }

    m_selectedCompanyId = id;

    // Emit signal to notify company selection change
    emit selectedCompanyIdChanged();

    qDebug() << "setSelectedCompanyId: Updated to ID:" << m_selectedCompanyId;
}

// Toggle contract selection (single/multi)
Q_INVOKABLE void ContractModel::toggleContractSelection(int contractId) {
    if (contractId < 0) {  // Prevent invalid contract IDs
        qWarning() << "toggleContractSelection: Invalid contract ID:" << contractId;
        return;
    }

    bool wasSelected = m_selectedContractIds.contains(contractId);

    if (wasSelected) {
        m_selectedContractIds.removeAll(contractId);  // Deselect contract (could use removeOne() as well)
        qDebug() << "toggleContractSelection: Removed contract ID:" << contractId;
    } else {
        m_selectedContractIds.append(contractId);  // Select contract
        qDebug() << "toggleContractSelection: Added contract ID:" << contractId;
    }

    // Only emit signal if the selection actually changed
    if (wasSelected != m_selectedContractIds.contains(contractId)) {
        emit selectedContractChanged();
        qDebug() << "toggleContractSelection: Updated selected contracts:" << m_selectedContractIds;
    }
}

// Return a selected contract ID
Q_INVOKABLE int ContractModel::getSingleSelectedContract() const {
    if (m_selectedContractIds.size() == 1) {
        return QList<int>(m_selectedContractIds).first(); // Convert to list if needed
    }

    return -1; // Return -1, if no valid single selection
}

// Return a list of selected contract IDs as QVariantList for QML
Q_INVOKABLE QVariantList ContractModel::getSelectedContracts() const {
    qDebug() << "getSelectedContracts() called. Selected contracts:" << m_selectedContractIds;

    QVariantList contractList;
    for (int contractId : m_selectedContractIds) {
        contractList.append(contractId);
    }

    if (contractList.isEmpty()) {
        qWarning() << "getSelectedContracts(): No contracts selected!";
    }

    return contractList;
}

// Clear all selected contracts
Q_INVOKABLE void ContractModel::clearSelectedContracts() {
    if (m_selectedContractIds.isEmpty()) {
        return;  // Skip unnecessary signal emission
    }

    m_selectedContractIds.clear();
    emit selectedContractChanged();  // Notify QML and backend

    qDebug() << "clearSelectedContracts: Cleared contract selection.";
}

// Check if a contract is selected
Q_INVOKABLE bool ContractModel::isContractSelected(int contractId) const {
    return m_selectedContractIds.contains(contractId);
}

// Return the most recently saved contract ID (the last contract added for a given company)
Q_INVOKABLE int ContractModel::selectMostRecentContract(int companyId) {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "selectMostRecentContract: Invalid company ID:" << companyId;
        return -1; // Return -1 if no valid company exists
    }

    const QList<ContractCondition> &contracts = m_companyDataCache[companyId].contracts;

    if (contracts.isEmpty()) {
        qWarning() << "selectMostRecentContract: No contracts found for company ID:" << companyId;
        return -1; // Return -1 if no contracts exist
    }

    int latestContractId = contracts.last().contractId;  // Assuming latest is at the end
    qDebug() << "selectMostRecentContract: Selected contract ID:" << latestContractId << "for company ID:" << companyId;

    return latestContractId; // Return the latest added contract ID
}

/// HELPERS
Q_INVOKABLE bool ContractModel::canSaveContract(const QVariantMap &formState) const {
    // Ensure a valid company is selected
    if (selectedCompanyId() == -1) {
        qDebug() << "canSaveContract: No company selected, saving not allowed.";
        return false;
    }

    // Extract and trim string-based fields
    QString startYear = formState.value("startYear", "").toString().trimmed();
    QString percentages = formState.value("percentages", "").toString().trimmed();

    // Ensure valid multiPay option flag
    bool multiPay = formState.value("multiPay", false).toBool(); // If false, revenue is paid once. If true, revenue is split.

    // Allow threshold == 0 (optional)
    double threshold = formState.value("threshold", 0.0).toDouble();

    // Basic completeness and value range check (same as formHasContent but stricter)
    bool valid =
        formState.value("startMonth", 0).toInt() > 0 &&
        !startYear.isEmpty() && startYear.toInt() > 0 &&
        formState.value("initialRevenue", 0.0).toDouble() > 0.0 &&
        threshold >= 0.0 &&
        formState.value("delay", 0).toInt() >= 0 &&
        (!multiPay || formState.value("secondDelay", 0).toInt() >= 0) &&  // // Only relevant when multiPay == true
        !percentages.isEmpty();
    // const QString formula = tempContractEdit.value("formula", "").toString().trimmed();
    // const bool isFormulaValid = formula.isEmpty() || isValidFormula(formula);  // Implement `isValidFormula()`

    qDebug() << "canSaveContract: Basic completeness check result:" << valid;

    return valid;
}

//  Determine if the Generate Revenue button should be enabled
Q_INVOKABLE bool ContractModel::canGenerateRevenue(const QVariantMap &formState) const {
    // Ensure a valid company is selected
    if (selectedCompanyId() == -1) {
        qDebug() << "canGenerateRevenue: No company selected. Revenue generation not allowed.";
        return false;
    }

    // Extract and trim string-based fields
    QString startYear = formState.value("startYear", "").toString().trimmed();
    QString percentages = formState.value("percentages", "").toString().trimmed();

    // Ensure valid multiPay option flag
    bool multiPay = formState.value("multiPay", false).toBool();

    // Allow threshold == 0 (optional)
    double threshold = formState.value("threshold", 0.0).toDouble();

    // Basic completeness and value range check (same logic as canSaveContract())
    bool valid =
        formState.value("startMonth", 0).toInt() > 0 &&
        !startYear.isEmpty() && startYear.toInt() > 0 &&
        formState.value("initialRevenue", 0.0).toDouble() > 0.0 &&
        threshold >= 0.0 &&
        formState.value("delay", 0).toInt() >= 0 &&
        (!multiPay || formState.value("secondDelay", 0).toInt() >= 0) &&  // Only relevant when multiPay == true
        !percentages.isEmpty();

    qDebug() << "canGenerateRevenue: Basic completeness check result:" << valid;

    return valid;
}

Q_INVOKABLE bool ContractModel::ensureContractSavedBeforeRevenue(int &contractId) {
    if (contractId != -1) {
        qDebug() << "ensureContractSavedBeforeRevenue: Contract already exists, ID:" << contractId;

        return true;
    }

    qDebug() << "ensureContractSavedBeforeRevenue: No saved contract found. Proceeding with saving...";

    // Save the contract
    saveContract();

    // Retrieve the new contract ID
    contractId = getCurrentContractId();
    if (contractId == -1) {
        qWarning() << "ensureContractSavedBeforeRevenue: Contract ID still -1 after saving. Attempting to reload...";

        // Try reloading the most recent contract
        contractId = selectMostRecentContract(selectedCompanyId());
        if (contractId == -1) {
            qWarning() << "ensureContractSavedBeforeRevenue: Contract ID is still -1 after reloading.";

            return false;
        }
    }

    qDebug() << "ensureContractSavedBeforeRevenue: Contract saved successfully, new ID:" << contractId;

    return true;
}

// Validation for QML (string input like "40/60/0", exposed to QML for direct user input validation)
Q_INVOKABLE bool ContractModel::validatePercentageString(const QString &percentages) const {
    QString trimmedPercentages = percentages.trimmed();

    if (trimmedPercentages.isEmpty()) {
        qWarning() << "validatePercentageString: Empty input.";
        return false;
    }

    QStringList parts = trimmedPercentages.split("/");
    QList<double> percentageValues;
    double total = 0;

    for (const QString &part : parts) {
        bool ok;
        double value = part.trimmed().toDouble(&ok);

        if (!ok || value <= 0) { // Allow 0% but prevent negative values
            qWarning() << "validatePercentageString: Invalid percentage:" << part;
            return false;
        }

        percentageValues.append(value);
        total += value;
    }

    // Ensure total sum is exactly 100%
    if (!qFuzzyCompare(total + 1, 101.0)) {  // Safer float comparison for precision issues
        qWarning() << "validatePercentageString: Total percentages must equal 100%. Current total:" << total;
        return false;
    }

    // If only one value is provided, it must be exactly 100 (valid for single-payment contracts)
    if (percentageValues.size() == 1 && percentageValues[0] != 100.0) {
        qWarning() << "validatePercentageString: Single percentage must be exactly 100% if multi-pay is not used.";
        return false;
    }

    // If multi-pay is used, ensure two valid percentages exist
    if (percentageValues.size() == 2) {
        double firstValue = percentageValues[0];
        double secondValue = percentageValues[1];

        // Allow "X/Y" or "Y/X" but require total = 100%
        qDebug() << "validatePercentageString: Validated split as" << firstValue << "/" << secondValue;
    } else if (percentageValues.size() > 2) {
        qWarning() << "validatePercentageString: Too many values in percentage split.";
        return false;
    }

    return true;
}

// Getter for validation error message
Q_INVOKABLE QString ContractModel::lastValidationError() const {
    return m_lastValidationError;
}

// // Getter
// Q_INVOKABLE bool ContractModel::isGeneratingRevenue() const {
//     return m_isGeneratingRevenue;
// }

// // Ensuring signal emission
// void ContractModel::setGeneratingRevenue(bool isGenerating) {
//     if (m_isGeneratingRevenue != isGenerating) {
//         m_isGeneratingRevenue = isGenerating;
//         emit isGeneratingRevenueChanged();
//     }
// }

/// GETTERS AND SETTERS FOR CONTRACT FORM FIELDS
// Get start month for a contract
Q_INVOKABLE int ContractModel::getStartMonth(int companyId, int index) const {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "getStartMonth: Invalid company ID:" << companyId;
        return -1; // Default empty value
    }

    // Handle case where a new contract is being created (contractId == -1)
    if (index == -1) {
        qDebug() << "getStartMonth: Returning temporary form value for new contract.";
        return tempContractEdit.value("startMonth", -1).toInt();
    }

    const QList<ContractCondition> &contracts = m_companyDataCache[companyId].contracts;

    if (index < 0 || index >= contracts.size()) {
        qWarning() << "getStartMonth: Invalid contract index for company:" << companyId << "Index:" << index;
        return -1; // No contracts yet, return default
    }

    int value = contracts[index].startMonth;
    qDebug() << "getStartMonth: Returning start month:" << value << "for Company ID:" << companyId << "Contract Index:" << index;
    return value;
}

// Set start month for a contract
Q_INVOKABLE void ContractModel::setStartMonth(int companyId, int startMonth) {
    if (!m_companyDataCache.contains(companyId) || startMonth < 1 || startMonth > 12) {
        qWarning() << "setStartMonth: Invalid company ID or start month. Company ID:" << companyId << "Start Month:" << startMonth;
        return;
    }

    // Handle new contract case
    if (getSingleSelectedContract() == -1) {
        qDebug() << "setStartMonth: Storing start month in temporary contract: " << startMonth;
        tempContractEdit["startMonth"] = startMonth;
        emit formChanged();
        return;
    }

    // Handle existing contract case
    if (!m_companyDataCache[companyId].contracts.isEmpty()) {
        m_companyDataCache[companyId].contracts[0].startMonth = startMonth;
        qDebug() << "setStartMonth: Updated start month to: " << startMonth << " for Company ID: " << companyId;

        emit contractChanged(companyId);
        emit formChanged();
    } else {
        qWarning() << "setStartMonth: No contracts exist for company ID: " << companyId;
    }
}

// Get start year for a contract
Q_INVOKABLE int ContractModel::getStartYear(int companyId, int index) const {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "getStartYear: Invalid company ID:" << companyId;
        return -1; // Default empty value
    }

    // Handle case where a new contract is being created (contractId == -1)
    if (index == -1) {
        qDebug() << "getStartYear: Returning temporary form value for new contract.";
        return tempContractEdit.value("startYear", 0).toInt();
    }

    const QList<ContractCondition> &contracts = m_companyDataCache[companyId].contracts;

    if (index < 0 || index >= contracts.size()) {
        qWarning() << "getStartYear: Invalid contract index for company:" << companyId << "Index:" << index;
        return -1; // No contracts yet, return default
    }

    int value = contracts[index].startYear;
    qDebug() << "getStartYear: Returning start year:" << value << "for Company ID:" << companyId << "Contract Index:" << index;
    return value;
}

// Set start year for a contract
Q_INVOKABLE void ContractModel::setStartYear(int companyId, int startYear) {
    if (!m_companyDataCache.contains(companyId) || startYear <= 0) {
        qWarning() << "setStartYear: Invalid company ID or start year. Company ID:" << companyId << "Start Year:" << startYear;
        return;
    }

    // Handle new contract case
    if (getSingleSelectedContract() == -1) {
        qDebug() << "setStartYear: Storing start year in temporary contract: " << startYear;
        tempContractEdit["startYear"] = startYear;
        emit formChanged();
        return;
    }

    // Handle existing contract case
    if (!m_companyDataCache[companyId].contracts.isEmpty()) {
        m_companyDataCache[companyId].contracts[0].startYear = startYear;
        qDebug() << "setStartYear: Updated start year to: " << startYear << " for Company ID: " << companyId;

        emit contractChanged(companyId);
        emit formChanged();
    } else {
        qWarning() << "setStartYear: No contracts exist for company ID: " << companyId;
    }
}

// Get initial revenue for a contract
Q_INVOKABLE double ContractModel::getInitialRevenue(int companyId, int index) const {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "getInitialRevenue: Invalid company ID:" << companyId;
        return 0.0; // Default empty value
    }

    // Handle case where a new contract is being created (contractId == -1)
    if (index == -1) {
        qDebug() << "getInitialRevenue: Returning temporary form value for new contract.";
        return tempContractEdit.value("initialRevenue", 0.0).toDouble();
    }

    const QList<ContractCondition> &contracts = m_companyDataCache[companyId].contracts;

    if (index < 0 || index >= contracts.size()) {
        qWarning() << "getInitialRevenue: Invalid contract index for company:" << companyId << "Index:" << index;
        return 0.0; // No contracts yet, return default
    }

    double value = contracts[index].initialRevenue;
    qDebug() << "getInitialRevenue: Returning initial revenue:" << value << "for Company ID:" << companyId << "Contract Index:" << index;
    return value;
}

// Set initial revenue for a contract
Q_INVOKABLE void ContractModel::setInitialRevenue(int companyId, double initialRevenue) {
    if (!m_companyDataCache.contains(companyId) || initialRevenue < 0) {
        qWarning() << "setInitialRevenue: Invalid company ID or revenue. Company ID:" << companyId << "Initial Revenue:" << initialRevenue;
        return;
    }

    // Handle new contract case
    if (getSingleSelectedContract() == -1) {
        qDebug() << "setInitialRevenue: Storing initial revenue in temporary contract: " << initialRevenue;
        tempContractEdit["initialRevenue"] = initialRevenue;
        emit formChanged();
        return;
    }

    // Handle existing contract case
    if (!m_companyDataCache[companyId].contracts.isEmpty()) {
        m_companyDataCache[companyId].contracts[0].initialRevenue = initialRevenue;
        qDebug() << "setInitialRevenue: Updated initial revenue to: " << initialRevenue
                 << " for Company ID: " << companyId;

        emit contractChanged(companyId);
        emit formChanged();
    } else {
        qWarning() << "setInitialRevenue: No contracts exist for company ID: " << companyId;
    }
}

// Get threshold for a contract
Q_INVOKABLE double ContractModel::getThreshold(int companyId, int index) const {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "getThreshold: Invalid company ID:" << companyId;
        return 0.0; // Default empty value
    }

    // Handle new contract case
    if (index == -1) {
        qDebug() << "getThreshold: Returning temporary form value for new contract.";
        return tempContractEdit.value("threshold", 0.0).toDouble();
    }

    const QList<ContractCondition> &contracts = m_companyDataCache[companyId].contracts;

    if (index < 0 || index >= contracts.size()) {
        qWarning() << "getThreshold: Invalid contract index for company:" << companyId << "Index:" << index;
        return 0.0; // No contracts yet, return default
    }

    double value = contracts[index].threshold;
    qDebug() << "getThreshold: Returning threshold:" << value << "for Company ID:" << companyId << "Contract Index:" << index;
    return value;
}

// Set threshold for a contract
Q_INVOKABLE void ContractModel::setThreshold(int companyId, double threshold) {
    if (!m_companyDataCache.contains(companyId) || threshold < 0) {
        qWarning() << "setThreshold: Invalid company ID or threshold value. Company ID:" << companyId << "Threshold:" << threshold;
        return;
    }

    // Handle new contract case
    if (getSingleSelectedContract() == -1) {
        qDebug() << "setThreshold: Storing threshold in temporary contract.";
        tempContractEdit["threshold"] = threshold;
        emit formChanged();
        return;
    }

    // Handle existing contract case
    if (!m_companyDataCache[companyId].contracts.isEmpty()) {
        m_companyDataCache[companyId].contracts[0].threshold = threshold; // Update the first contract
        qDebug() << "setThreshold: Updated threshold to:" << threshold << "for Company ID:" << companyId;

        emit contractChanged(companyId);
        emit formChanged();  // Ensure UI updates when the value changes
    } else {
        qWarning() << "setThreshold: No contracts exist for company ID:" << companyId;
    }
}

// Get delay for a contract
Q_INVOKABLE int ContractModel::getDelay(int companyId, int index) const {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "getDelay: Invalid company ID:" << companyId;
        return 0; // Default empty value
    }

    // Handle new contract case
    if (index == -1) {
        qDebug() << "getDelay: Returning temporary form value for new contract.";
        return tempContractEdit.value("delay", 0).toInt();
    }

    const QList<ContractCondition> &contracts = m_companyDataCache[companyId].contracts;

    if (index < 0 || index >= contracts.size()) {
        qWarning() << "getDelay: Invalid contract index for company:" << companyId << "Index:" << index;
        return 0; // No contracts yet, return default
    }

    int value = contracts[index].delay;
    qDebug() << "getDelay: Returning delay:" << value << "for Company ID:" << companyId << "Contract Index:" << index;
    return value;
}

// Set delay for a contract
Q_INVOKABLE void ContractModel::setDelay(int companyId, int delay) {
    if (!m_companyDataCache.contains(companyId) || delay < 0) {
        qWarning() << "setDelay: Invalid company ID or delay value. Company ID:" << companyId << "Delay:" << delay;
        return;
    }

    // Handle new contract case
    if (getSingleSelectedContract() == -1) {
        qDebug() << "setDelay: Storing delay in temporary contract: " << delay;
        tempContractEdit["delay"] = delay;
        emit formChanged();
        return;
    }

    // Handle existing contract case
    if (!m_companyDataCache[companyId].contracts.isEmpty()) {
        m_companyDataCache[companyId].contracts[0].delay = delay;
        qDebug() << "setDelay: Updated delay to: " << delay << " for Company ID: " << companyId;

        emit contractChanged(companyId);
        emit formChanged();
    } else {
        qWarning() << "setDelay: No contracts exist for company ID: " << companyId;
    }
}

bool ContractModel::getMultiPay() const {
    return tempContractEdit.value("multiPay", false).toBool();
}

void ContractModel::setMultiPay(bool value) {
    if (tempContractEdit["multiPay"] != value) {
        tempContractEdit["multiPay"] = value;
        emit formChanged();
    }
}

int ContractModel::getSecondDelay() const {
    return tempContractEdit.value("secondDelay", 0).toInt();
}

void ContractModel::setSecondDelay(int value) {
    if (value < 0) {
        qWarning() << "setSecondDelay: Invalid negative value!";
        return;
    }

    if (tempContractEdit["secondDelay"] != value) {
        tempContractEdit["secondDelay"] = value;
        emit formChanged();
    }
}

// Get percentages as formatted string
Q_INVOKABLE QString ContractModel::getPercentages(int companyId, int index) const {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "getPercentages: Invalid company ID:" << companyId;
        return ""; // Default empty string
    }

    // Handle new contract case
    if (index == -1) {
        QString tempValue = tempContractEdit.value("percentages", "").toString();
        qDebug() << "getPercentages: Returning temporary form value for new contract: " << tempValue;
        return tempValue;
    }

    const QList<ContractCondition> &contracts = m_companyDataCache[companyId].contracts;

    if (index < 0 || index >= contracts.size()) {
        qWarning() << "getPercentages: Invalid contract index for company: " << companyId << " Index: " << index;
        return ""; // No contracts yet, return empty string
    }

    // Convert percentages list to a formatted string (e.g. "40/60")
    QString formattedValue = formatPercentageListToString(contracts[index].percentages);
    qDebug() << "getPercentages: Returning formatted percentages: " << formattedValue
             << " for Company ID: " << companyId << " Contract Index: " << index;

    return formattedValue;
}

// Set percentages (formatted string -> QList<double>)
Q_INVOKABLE void ContractModel::setPercentages(int companyId, const QString &percentages) {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "setPercentages: Invalid company ID.";
        return;
    }

    QString trimmedPercentages = percentages.trimmed();

    if (!validatePercentageString(trimmedPercentages)) {
        qWarning() << "setPercentages: Invalid percentages. Not updating.";
        return;
    }

    QList<double> parsedPercentages = parsePercentageStringToList(trimmedPercentages);
    if (parsedPercentages.isEmpty()) {
        qWarning() << "setPercentages: Failed to parse percentages. Input: " << trimmedPercentages;
        return;
    }

    // Handle new contract case (store in temporary storage)
    if (getSingleSelectedContract() == -1) {
        qDebug() << "setPercentages: Storing percentages in temporary contract: " << trimmedPercentages;
        tempContractEdit["percentages"] = trimmedPercentages;
        emit formChanged();  // UI update for new contract form
        return;
    }

    // Handle existing contract case
    if (!m_companyDataCache[companyId].contracts.isEmpty()) {
        m_companyDataCache[companyId].contracts[0].percentages = parsedPercentages;
        qDebug() << "setPercentages: Updated percentages to: " << trimmedPercentages
                 << " for Company ID: " << companyId;

        emit contractChanged(companyId);  // Notify other parts of the application
        emit formChanged();  // Ensure UI updates when the value changes
    } else {
        qWarning() << "setPercentages: No contracts exist for company ID: " << companyId;
    }
}

/// HANDLERS FOR CONTRACT OPERATIONS
// Handle updated contract for an individual company
void ContractModel::handleContractChanged(int companyId) {
    // Check if the company exists
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "handleContractChanged: No company found with ID:" << companyId << ". Ignoring the change.";
        return;
    }

    // Ensure there are valid contracts for this company
    const QList<ContractCondition> &contracts = m_companyDataCache[companyId].contracts;
    if (contracts.isEmpty()) {
        qWarning() << "handleContractChanged: No contracts for company ID:" << companyId << ". Revenue not recalculated.";
        return;
    }

    qDebug() << "handleContractChanged: Recalculating revenue for all contracts of Company ID:" << companyId;

    // Iterate through all contracts and recalculate revenue
    for (const auto &contract : contracts) {
        int contractId = contract.contractId;

        // Check if recalculation is needed
        bool shouldRecalculate = !m_revenueData.contains(companyId) ||
                                 !m_revenueData[companyId].contains(contractId);

        if (shouldRecalculate) {
            qDebug() << "handleContractChanged: Recalculating revenue for Contract ID:" << contractId;
            m_revenueData[companyId][contractId] = calculateRevenue(companyId, contractId);

            // Emit revenue update signal for QML/UI updates
            emit revenueGenerated(contractId);
        }
    }

    // Notify QML or other components about the contract change
    emit contractChanged(companyId);

    qDebug() << "handleContractChanged: Contract updated for company ID:" << companyId << ". Revenue recalculated.";
}

// Handle bulk contract updates
void ContractModel::handleContractsReloaded() {
    qDebug() << "handleContractsReloaded: Bulk contract reload initiated.";

    // Ensure cache consistency by logging contract counts
    for (auto it = m_companyDataCache.constBegin(); it != m_companyDataCache.constEnd(); ++it) {
        qDebug() << "Company ID:" << it.key() << "has" << it.value().contracts.size() << "contracts.";
    }

    // Iterate over all companies in the cache
    for (auto it = m_companyDataCache.constBegin(); it != m_companyDataCache.constEnd(); ++it) {
        int companyId = it.key();
        const QList<ContractCondition> &contracts = it.value().contracts;

        // Skip companies with no contracts (preventing unnecessary processing)
        if (contracts.isEmpty()) {
            qWarning() << "handleContractsReloaded: No contracts found for Company ID:" << companyId;
            continue;
        }

        qDebug() << "handleContractsReloaded: Reloading revenue for Company ID:" << companyId;

        // Ensure `m_revenueData[companyId]` is initialized properly
        if (!m_revenueData.contains(companyId)) {
            m_revenueData[companyId] = QMap<int, QMap<int, QMap<int, double>>>(); // Initialize company entry
        }

        // Process each contract within this company
        for (const auto &contract : contracts) {
            int contractId = contract.contractId;

            // Ensure `m_revenueData[companyId][contractId]` is initialized properly
            if (!m_revenueData[companyId].contains(contractId)) {
                m_revenueData[companyId][contractId] = QMap<int, QMap<int, double>>(); // Initialize contract entry
            }

            // Retrieve the calculated revenue breakdown
            QMap<int, QMap<int, double>> calculatedRevenue = calculateRevenue(companyId, contractId);

            // Store the entire revenue breakdown in `m_revenueData`
            m_revenueData[companyId][contractId] = calculatedRevenue;

            // Emit revenue signal to notify the UI
            emit revenueGenerated(contractId);
        }
    }

    // Notify QML and other components that contracts have been reloaded
    emit contractsReloaded();

    qDebug() << "handleContractsReloaded: Bulk contract reload completed. Revenue recalculated.";
}

/// HANDLERS FOR COMPANY OPERATIONS
// Handle add company
void ContractModel::handleCompanyAdded(int companyId, const QString &name) {
    // Validate name
    if (!isNameValid(name)) {
        qWarning() << "handleCompanyAdded: Invalid company name provided:" << name;
        return;
    }

    // Add the company to the cache (without reloading everything)
    m_companyDataCache[companyId] = {companyId, name}; // Initialize with no contracts

    emit companyCacheUpdated(); // Notify UI

    qDebug() << "handleCompanyAdded: Company added with ID:" << companyId << "Name:" << name;
}

// Handle edit company
void ContractModel::handleCompanyEdited(int companyId, const QString &newName) {
    // Validate name
    if (!isNameValid(newName)) {
        qWarning() << "handleCompanyEdited: Invalid name provided:" << newName;
        return;
    }

    // Update only the company name in the cache
    if (m_companyDataCache.contains(companyId)) {
        m_companyDataCache[companyId].name = newName;
    } else {
        qWarning() << "handleCompanyEdited: Company ID not found in cache. Ignoring.";
        return;
    }

    emit companyCacheUpdated(); // Notify UI

    qDebug() << "handleCompanyEdited: Company with ID:" << companyId << "updated to:" << newName;
}

// Handle remove company
void ContractModel::handleCompanyRemoved(int companyId) {
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "handleCompanyRemoved: No company found with ID:" << companyId << ". Ignoring.";
        return;
    }

    // Remove company data
    m_companyDataCache.remove(companyId);
    m_revenueData.remove(companyId);
    m_revenueSummary.clear();
    m_lastValidationError.clear();

    if (m_selectedCompanyId == companyId) {
        m_selectedCompanyId = -1;
        tempContractEdit.clear();
        m_selectedContractIds.clear();

        emit selectedCompanyIdChanged();
        emit selectedContractChanged();
        emit formReset();
    }

    emit companyCacheUpdated();
    emit contractsReloaded();

    qDebug() << "handleCompanyRemoved: Successfully removed company with ID:" << companyId;

    // Log remaining state
    if (m_companyDataCache.isEmpty()) {
        qDebug() << "handleCompanyRemoved: No companies remain after deletion.";
    }
}

/// VALIDATION AND UTILITY FUNCTIONS
// Validate company name
bool ContractModel::isNameValid(const QString &name) const {
    QString trimmedName = name.trimmed();

    // Check for empty or whitespace name
    if (trimmedName.isEmpty()) {
        qWarning() << "isNameValid: Name is empty or whitespace.";
        return false;
    }

    return true;
}

// Validate percentages (internal validation, C++ side, handling numeric percentages)
bool ContractModel::validatePercentageList(const QList<double> &percentages, bool multiPay) const {
    double total = 0;

    for (double percentage : percentages) {
        if (percentage < 0) {  // Allow 0, but prevent negative values
            qWarning() << "validatePercentageList: Invalid negative percentage:" << percentage;
            return false;
        }
        total += percentage;
    }

    // Ensure total sum is exactly 100% (not more, not less)
    if (!qFuzzyCompare(total + 1, 101.0)) {  // Safer float comparison
        qWarning() << "validatePercentageList: Total percentages must equal 100%. Current total:" << total;
        return false;
    }

    // If multi-pay is disabled, only allow a single percentage of exactly 100%
    if (!multiPay && (percentages.size() != 1 || percentages[0] != 100.0)) {
        qWarning() << "validatePercentageList: Single percentage must be exactly 100% if multi-pay is not used.";
        return false;
    }

    // If multi-pay is used, ensure two valid percentages exist
    // if (percentages.size() == 2) {
    //     double firstValue = percentages[0];
    //     double secondValue = percentages[1];
    //     // Allow "X/Y" or "Y/X" as long as sum = 100%
    //     qDebug() << "validatePercentageList: Validated split as" << firstValue << "/" << secondValue;
    // } else if (percentages.size() > 2) {
    //     qWarning() << "validatePercentageList: Too many values in percentage split.";
    //     return false;
    // }

    // If multi-pay is enabled, there must be two valid percentages
    if (multiPay && percentages.size() != 2) {
        qWarning() << "validatePercentageList: Multi-pay contracts must have two percentages.";
        return false;
    }

    if (multiPay) {
        qDebug() << "validatePercentageList: Multi-payment validated as " << percentages[0] << "/" << percentages[1];
    }

    return true;
}

// Parse percentages to bind (converts QString to QList<double>, used when retrieving data from the database)
QList<double> ContractModel::parsePercentageStringToList(const QString &percentagesStr) const {
    QList<double> percentages;
    QStringList parts = percentagesStr.split("/", Qt::SkipEmptyParts);

    for (const QString &part : parts) {
        bool ok = false;
        double value = part.trimmed().toDouble(&ok);

        if (ok) {
            percentages.append(value);
        } else {
            qWarning() << "parsePercentageStringToList: Failed to convert part to double:" << part;
        }
    }

    // If only "100" is entered, treat it as a single-payment case
    if (percentages.size() == 1 && percentages[0] == 100.0) {
        qDebug() << "parsePercentageStringToList: Detected full single payment.";
        return {100.0};  // Single-payment case
    }

    qDebug() << "parsePercentageStringToList: Parsed percentages:" << percentages;
    return percentages;
}


// Format percentages (converts QList<double> to QString, used when storing or displaying data in QML or SQL queries)
QString ContractModel::formatPercentageListToString(const QList<double> &percentages) const {
    // If only "100" is present, return it directly
    if (percentages.size() == 1 && qFuzzyCompare(percentages[0], 100.0)) {
        return "100";
    }

    QStringList formatted;
    for (double percentage : percentages) {
        // Whole numbers are stored as integers, decimals keep two decimal places
        if (qFuzzyCompare(percentage, std::floor(percentage))) {
            formatted.append(QString::number(static_cast<int>(percentage)));
        } else {
            formatted.append(QString::number(percentage, 'f', 2));  // Two decimal places if needed
        }
    }

    QString result = formatted.join("/");
    qDebug() << "formatPercentageListToString: Formatted percentages:" << result;
    return result;
}


// Validate form input
Q_INVOKABLE bool ContractModel::validateCurrentForm() const {
    m_lastValidationError.clear();
    QStringList errors;

    const auto &contract = tempContractEdit;

    auto safeToInt = [](const QVariant &value, int defaultValue = -1) {
        bool ok = false;
        int result = value.toInt(&ok);
        return ok ? result : defaultValue;
    };

    auto safeToDouble = [](const QVariant &value, double defaultValue = -1.0) {
        bool ok = false;
        double result = value.toDouble(&ok);
        return ok ? result : defaultValue;
    };

    // Required field checks
    int startMonth = safeToInt(contract["startMonth"]);
    if (startMonth < 1 || startMonth > 12) {
        errors << tr("• Start month must be between 1 and 12.");
    }

    int startYear = safeToInt(contract["startYear"]);
    if (startYear <= 0) {
        errors << tr("• Start year must be a positive integer (e.g. 2024).");
    }

    double initialRevenue = safeToDouble(contract["initialRevenue"]);
    if (initialRevenue < 0.0) {
        errors << tr("• Initial revenue must be zero or greater.");
    }

    double threshold = safeToDouble(contract["threshold"]);
    if (threshold < 0.0) {
        errors << tr("• Threshold must be zero or greater.");
    }

    int delay = safeToInt(contract["delay"]);
    if (delay < 0) {
        errors << tr("• First payment delay must be zero or greater.");
    }

    // Validate multiPay field (must be boolean)
    bool multiPay = contract.value("multiPay", false).toBool();

    // Validate secondDelay, only if multiPay is enabled
    int secondDelay = safeToInt(contract["secondDelay"]);
    if (multiPay && secondDelay < 0) {
        errors << tr("• Second payment delay must be zero or greater if multi-pay is enabled.");
    }

    // Validate percentages
    QString percentages = contract.value("percentages").toString().trimmed();
    QStringList percentageParts = percentages.split("/");

    if (!validatePercentageString(percentages)) {
        errors << tr("• Percentages format is invalid or does not sum to 100%. Use format like '40/60'.");
    }

    // - If multi-pay is disabled, the user should only enter "100" (not "100/0" or "0/100").
    // - If multi-pay is enabled, there should be **two** values.
    if (!multiPay && percentageParts.size() == 2) {
        errors << tr("• If multi-pay is disabled, enter '100' instead of '100/0' or '0/100'.");
    }
    if (multiPay && percentageParts.size() != 2) {
        errors << tr("• If multi-pay is enabled, percentages must be split (e.g., '40/60').");
    }

    // Optional formula logic - uncomment if formula editing is reintroduced
    /*
    QString formula = contract.value("formula").toString().trimmed();
    if (!formula.isEmpty() && !isValidFormula(formula)) {
        errors << "• Formula is not valid. Please review the formula syntax.";
    }
    */

    if (!errors.isEmpty()) {
        m_lastValidationError =
            tr("The form has the following problems:\n\n") +
            errors.join("\n") +
            tr("\n\nPlease correct them and try again.");

        qWarning() << "validateCurrentForm: Validation failed with errors:" << errors;
        return false;
    }

    qDebug() << "validateCurrentForm: All form fields are valid.";
    return true; // All checks pass
}

/// REVENUE BREAKDOWN
Q_INVOKABLE bool ContractModel::prepareAndGenerateRevenue(const QVariantMap &formData) {
    m_lastValidationError.clear(); // Reset validation errors
    qDebug() << "prepareAndGenerateRevenue: Validating revenue generation conditions...";

    // Basic completeness check
    if (!canGenerateRevenue(formData)) {
        m_lastValidationError = tr("Please complete all required fields with valid values before generating revenue.");
        qDebug() << "prepareAndGenerateRevenue: Basic validation failed.";
        return false;
    }

    const bool isEditing = getCurrentContractId() > 0;
    if (!isEditing) {
        qDebug() << "prepareAndGenerateRevenue: Initializing new tempContractEdit...";
        initializeTempContractEdit(selectedCompanyId(), -1);
    }

    // Apply form data to temporary contract storage
    applyFormToTempContract(formData);

    // Perform deep validation
    if (!validateCurrentForm()) {
        qWarning() << "prepareAndGenerateRevenue: Form validation failed - " << m_lastValidationError;
        return false;
    }

    // Ensure the contract is saved
    // int contractId = formData.value("contractId", -1).toInt();
    int contractId = getCurrentContractId();
    if (!ensureContractSavedBeforeRevenue(contractId)) {
        qWarning() << "prepareAndGenerateRevenue: Contract saving failed.";
        return false;
    }

    // qDebug() << "prepareAndGenerateRevenue: Generating revenue for Company ID:" << selectedCompanyId()
    //          << " Contract ID:" << contractId;

    generateRevenue(selectedCompanyId(), contractId);

    // emit formReset();
    return true;
}

// Create revenue breakdown when user clicks the button and store it in cache
Q_INVOKABLE void ContractModel::generateRevenue(int companyId, int contractId) {
    // Validate contract
    if (contractId == -1) {
        qWarning() << "generateRevenue: Invalid contract ID (-1), aborting revenue generation.";
        return;
    }

    // Validate company
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "generateRevenue: Company ID not found:" << companyId;
        return;
    }

    // Locate the contract in cache (handle newly saved contracts)
    auto &companyContracts = m_companyDataCache[companyId].contracts;
    auto it = std::find_if(companyContracts.begin(), companyContracts.end(),
                           [contractId](const ContractCondition &c) { return c.contractId == contractId; });

    if (it == companyContracts.end()) {
        qWarning() << "generateRevenue: Contract ID not found in cache, attempting reload...";

        // Try to reload contracts from the database if missing
        loadContractsForCompany(companyId);

        // Search again
        it = std::find_if(companyContracts.begin(), companyContracts.end(),
                          [contractId](const ContractCondition &c) { return c.contractId == contractId; });

        if (it == companyContracts.end()) {
            qWarning() << "generateRevenue: Contract still not found after reload. Aborting revenue generation.";
            return;
        }
    }

    qDebug() << "generateRevenue: Processing revenue for Contract ID:" << contractId;

    // Check if recalculation is needed
    bool shouldRecalculate = !m_revenueData.contains(companyId) || !m_revenueData[companyId].contains(contractId);

    if (!shouldRecalculate) {
        qDebug() << "generateRevenue: Checking if contract details changed...";

        const ContractCondition &contract = *it;
        QVariantMap latestContractData = getContractById(companyId, contractId);
        // QMap<int, QMap<int, double>> newRevenue = calculateRevenue(companyId, contractId);

        if (latestContractData["startMonth"].toInt() != contract.startMonth ||
            latestContractData["startYear"].toInt() != contract.startYear ||
            latestContractData["initialRevenue"].toDouble() != contract.initialRevenue ||
            latestContractData["threshold"].toDouble() != contract.threshold ||
            latestContractData["delay"].toInt() != contract.delay ||
            latestContractData["secondDelay"].toInt() != contract.secondDelay ||
            latestContractData["multiPay"].toBool() != contract.multiPay ||
            parsePercentageStringToList(latestContractData["percentages"].toString()) != contract.percentages) {

            qDebug() << "generateRevenue: Contract details changed, recalculating revenue...";
            shouldRecalculate = true;
        } else if (!m_revenueData.contains(companyId) || !m_revenueData[companyId].contains(contractId)) {
            qDebug() << "generateRevenue: Revenue data missing, recalculating.";
            shouldRecalculate = true;
        } else {
            qDebug() << "generateRevenue: No changes detected, skipping recalculation.";
            return;
        }
    }

    // Only recalculate revenue if necessary
    if (shouldRecalculate) {
        qDebug() << "generateRevenue: Calculating revenue for contract ID:" << contractId;

        auto revenueResult = calculateRevenue(companyId, contractId);
        if (revenueResult.isEmpty()) {
            qWarning() << "generateRevenue: Calculation failed, no revenue generated.";
            return;
        }

        m_revenueData[companyId][contractId] = revenueResult;

        // Update the revenue summary cache
        updateRevenueSummaryCache();
    }

    emit revenueGenerated(contractId); // Emit signal to notify UI
}

// Calculate monthly and yearly revenue based on stored contract values
Q_INVOKABLE QMap<int, QMap<int, double>> ContractModel::calculateRevenue(int companyId, int contractId) {
    QMap<int, QMap<int, double>> revenueData;

    // Ensure company exists
    if (!m_companyDataCache.contains(companyId)) {
        qWarning() << "calculateRevenue: Company ID not found:" << companyId;
        return revenueData;
    }

    // Find the contract within the company
    const QList<ContractCondition>& contracts = m_companyDataCache[companyId].contracts;
    auto it = std::find_if(contracts.begin(), contracts.end(),
                           [contractId](const ContractCondition &c) { return c.contractId == contractId; });

    if (it == contracts.end()) {
        qWarning() << "calculateRevenue: Contract ID not found in company data:" << contractId;
        return revenueData;
    }

    // Extract contract details
    const ContractCondition& contract = *it;
    double initialRevenue = contract.initialRevenue;
    int startMonth = contract.startMonth;
    int startYear = contract.startYear;
    int firstDelay = contract.delay;
    int secondDelay = contract.secondDelay;
    double threshold = contract.threshold;
    bool multiPay = contract.multiPay;
    QList<double> percentages = contract.percentages;

    // Ensure valid split percentages
    double split1 = percentages.size() >= 1 ? percentages[0] / 100.0 : 1.0;
    double split2 = percentages.size() == 2 ? percentages[1] / 100.0 : 0.0;

    // Function to adjust month-year overflow
    auto adjustDate = [](int month, int year) -> QPair<int, int> {
        if (month > 12) {
            month -= 12;
            year++;
        }
        return {month, year};
    };

    // Calculate first payment date
    QPair<int, int> firstPaymentDate = adjustDate(startMonth + firstDelay, startYear);
    int firstYear = firstPaymentDate.second;
    int firstMonth = firstPaymentDate.first;

    // Case 1: no threshold, no multi-pay (single full payment)
    if (threshold == 0 && !multiPay) {
        revenueData[firstYear][firstMonth] = initialRevenue;
        return revenueData;
    }

    // Case 2: no threshold, multi-pay (normal split)
    if (threshold == 0 && multiPay) {
        QPair<int, int> secondPaymentDate = adjustDate(firstMonth + secondDelay, firstYear);
        int secondYear = secondPaymentDate.second;
        int secondMonth = secondPaymentDate.first;

        revenueData[firstYear][firstMonth] = initialRevenue * split1;
        if (split2 > 0) revenueData[secondYear][secondMonth] = initialRevenue * split2;
        return revenueData;
    }

    // Case 3: threshold exists, no multi-pay (pay 100% if below, otherwise full)
    if (threshold > 0 && !multiPay) {
        if (initialRevenue < threshold) {
            revenueData[firstYear][firstMonth] = initialRevenue;  // Pay full amount, if below threshold
        } else {
            revenueData[firstYear][firstMonth] = initialRevenue;  // Pay full amount, if above threshold
        }
        return revenueData;
    }

    // Case 4: threshold exists, multi-pay (adjust split based on threshold)
    if (threshold > 0 && multiPay) {
        QPair<int, int> secondPaymentDate = adjustDate(firstMonth + secondDelay, firstYear);
        int secondYear = secondPaymentDate.second;
        int secondMonth = secondPaymentDate.first;

        if (initialRevenue < threshold) {
            // Below threshold: apply normal split
            revenueData[firstYear][firstMonth] = initialRevenue * split1;
            if (split2 > 0) revenueData[secondYear][secondMonth] = initialRevenue * split2;
        } else {
            // Above threshold
            revenueData[firstYear][firstMonth] = initialRevenue * split1;
            if (split2 > 0) revenueData[secondYear][secondMonth] = initialRevenue * split2;
            // Above threshold: reverse split
            // revenueData[firstYear][firstMonth] = initialRevenue * split2;
            // if (split1 > 0) revenueData[secondYear][secondMonth] = initialRevenue * split1;
        }
    }

    return revenueData;
}

// Convert revenue breakdown to QVariantList for QML compatibility
Q_INVOKABLE QVariantList ContractModel::getRevenueAsVariantList(int companyId, int contractId) {
    QVariantList revenueList;

    // Validate existence of revenue data
    const auto revenueIt = m_revenueData.find(companyId);
    if (revenueIt == m_revenueData.end() || !revenueIt->contains(contractId)) {
        qWarning() << "getRevenueAsVariantList: No revenue data found for Company ID:" << companyId
                   << ", Contract ID:" << contractId
                   << ". Available contracts:" << (revenueIt != m_revenueData.end() ? revenueIt->keys() : QList<int>());

        return revenueList;
    }

    // Retrieve stored revenue data
    const auto &revenueData = (*revenueIt)[contractId];

    // Convert into a QVariantList for QML compatibility
    for (auto yearIt = revenueData.constBegin(); yearIt != revenueData.constEnd(); ++yearIt) {
        int year = yearIt.key();
        for (auto monthIt = yearIt.value().constBegin(); monthIt != yearIt.value().constEnd(); ++monthIt) {
            QVariantMap revenueEntry;
            revenueEntry["year"] = year;
            revenueEntry["month"] = monthIt.key();
            revenueEntry["revenue"] = monthIt.value();
            revenueList.append(revenueEntry);
        }
    }

    qDebug() << "getRevenueAsVariantList: Returning revenue breakdown for Company ID:" << companyId
             << ", Contract ID:" << contractId
             << ", Total Entries:" << revenueList.size();

    return revenueList;
}

void ContractModel::updateRevenueSummaryCache() {
    m_revenueSummary.clear();
    qDebug() << "Starting revenue summary update...";

    for (auto companyIt = m_companyDataCache.constBegin(); companyIt != m_companyDataCache.constEnd(); ++companyIt) {
        int companyId = companyIt.key();
        const CompanyData &company = companyIt.value();

        qDebug() << "Company" << companyId << ":" << company.name << "Contracts:" << company.contracts.size();

        for (const ContractCondition &contract : company.contracts) {
            int contractId = contract.contractId;

            if (!m_revenueData.contains(companyId)) {
                qDebug() << "  No revenue data for company" << companyId;
                continue;
            }

            if (!m_revenueData[companyId].contains(contractId)) {
                qDebug() << "  No revenue data for contract" << contractId;
                continue;
            }

            const auto &yearMap = m_revenueData[companyId][contractId];
            qDebug() << "  Processing contract" << contractId << "Years:" << yearMap.keys();

            for (auto yearIt = yearMap.constBegin(); yearIt != yearMap.constEnd(); ++yearIt) {
                int year = yearIt.key();
                const auto &monthMap = yearIt.value();

                for (auto monthIt = monthMap.constBegin(); monthIt != monthMap.constEnd(); ++monthIt) {
                    int month = monthIt.key();
                    double revenue = monthIt.value();

                    m_revenueSummary[companyId][year][month] += revenue;
                }
            }
        }
    }

    qDebug() << "updateRevenueSummaryCache: Populated revenue summary for" << m_revenueSummary.size() << "companies.";
}

QVariantList ContractModel::getYearlyRevenueSummary(int year) {
    qDebug() << "getYearlyRevenueSummary: Year requested:" << year;
    QVariantList summaryList;

    // Monthly totals across all companies
    QMap<int, double> totalPerMonth;
    for (int m = 1; m <= 12; ++m)
        totalPerMonth[m] = 0.0;

    for (auto it = m_companyDataCache.constBegin(); it != m_companyDataCache.constEnd(); ++it) {
        int companyId = it.key();
        const QString &companyName = it.value().name;

        // Skip if summary not available
        // if (!m_revenueSummary.contains(companyId) || !m_revenueSummary[companyId].contains(year))
        //     continue;

        if (!m_revenueSummary.contains(companyId)) {
            qDebug() << "No revenue summary for company" << companyId;
            continue;
        }
        if (!m_revenueSummary[companyId].contains(year)) {
            qDebug() << "No summary for company" << companyId << "for year" << year;
            continue;
        }

        const auto &monthlyMap = m_revenueSummary[companyId][year];

        QVariantMap companyRow;
        companyRow["companyId"] = companyId;
        companyRow["companyName"] = companyName;

        QVariantList monthlyRevenues;
        for (int m = 1; m <= 12; ++m) {
            double revenue = monthlyMap.value(m, 0.0);
            monthlyRevenues.append(revenue);
            totalPerMonth[m] += revenue;
        }

        companyRow["monthlyRevenues"] = monthlyRevenues;
        summaryList.append(companyRow);
    }

    // Append total row
    QVariantMap totalRow;
    totalRow["companyId"] = -1;
    totalRow["companyName"] = "Total";

    QVariantList totalMonthly;
    for (int m = 1; m <= 12; ++m)
        totalMonthly.append(totalPerMonth[m]);
    totalRow["monthlyRevenues"] = totalMonthly;

    summaryList.append(totalRow);
    return summaryList;
}

void ContractModel::saveCsvToFile(const QString &csvContent, const QString &filePath) {
    QString localPath = QUrl(filePath).isValid() ? QUrl(filePath).toLocalFile() : filePath;

    if (localPath.isEmpty()) {
        qWarning() << "saveCsvToFile: Empty or invalid file path.";
        return;
    }

    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << file.errorString();
        return;
    }

    QTextStream stream(&file);
    stream << csvContent;
    file.close();

    qDebug() << "CSV successfully saved to:" << QFileInfo(localPath).absoluteFilePath();
}

/// SETTINGS
Q_INVOKABLE void ContractModel::reloadFromDatabase() {
    qDebug() << "ContractModel::reloadFromDatabase: Triggered.";

    initializeCache();

    qDebug() << "ContractModel::reloadFromDatabase: Done.";
}
