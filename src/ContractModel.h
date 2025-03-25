#ifndef CONTRACTMODEL_H
#define CONTRACTMODEL_H

#include "QtSqlIncludes.h"
#include "CompanyListModel.h"
#include <QObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Structure for an individual contract condition
struct ContractCondition {
    int contractId;              // Contract ID
    int startMonth;              // Contract start month
    int startYear;               // Contract start year
    double initialRevenue;       // Initial revenue value
    double threshold;            // Payment threshold
    int delay;                   // First payment delay in months
    int secondDelay;             // Second payment delay (for multi-pay contracts)
    bool multiPay;               // Whether multi-payment is enabled
    QList<double> percentages;   // Revenue split percentages (e.g., {40, 60})
    QString formula;             // Optional formula
};

// Structure for company data
struct CompanyData {
    int companyId;                              // Company ID
    QString name;                               // Company name
    QList<ContractCondition> contracts = {};    // List of contracts for this company, default to an empty list
};

class ContractModel : public QObject {
    Q_OBJECT

    // State variables
    Q_PROPERTY(int selectedCompanyId READ selectedCompanyId WRITE setSelectedCompanyId NOTIFY selectedCompanyIdChanged) // The primary key for accessing data in m_companyCache (used for retrieving or updating fields)
    // Q_PROPERTY(bool canSaveContract READ canSaveContract NOTIFY formChanged)

public:
    // Constructor
    explicit ContractModel(CompanyListModel *companyModel, QSqlDatabase dbConnection, QObject *parent = nullptr);

    // Form state management (tempContractEdit: backend buffer)
    Q_INVOKABLE QVariantMap getTempContract();                                          // Provide the current temporary contract being edited (if any)
    Q_INVOKABLE void updateTempContract(QString key, QVariant value);                   // Update a single field in the temp contract (manual field-level updates)
    Q_INVOKABLE void applyFormToTempContract(const QVariantMap &formData);              // Bulk-apply full form data to temp contract before saving

    // Contract data access (read-only for form loading)
    Q_INVOKABLE QVariantMap getContractById(int companyId, int contractId);             // Fetch a contract directly from the cache (read-only)

    // Contract lifecycle management (CRUD and reset)
    Q_INVOKABLE void saveContract();                                                    // Save the current temp contract (new or update decision handled inside)
    Q_INVOKABLE bool saveNewContract();                                                 // Save new contract into the database and update cache
    Q_INVOKABLE bool updateExistingContract(int contractId);                            // Update existing contract in the database and cache
    Q_INVOKABLE void removeContract(int companyId, int contractId);                     // Remove a contract from both database and cache

    Q_INVOKABLE int getCurrentContractId() const; // Tracks the current contract being edited (returns -1 for new contracts)
    bool hasFormChanged(const QVariantMap &formData) const;  // Checks if the form has changed (compares current formData to tempContractEdit)
    Q_INVOKABLE bool prepareAndSaveContract(const QVariantMap &formData); // Central save handler (combines completeness check, change check, validation, and saving)
    Q_INVOKABLE bool canProceedWithSave(const QVariantMap &formData) const;  // Unified check for Save button

    // Internal form buffer (tempContractEdit) initialization
    Q_INVOKABLE void initializeTempContractEdit(int companyId, int contractId = -1);    // Initialize tempContractEdit with full schema (defaults filled)

    Q_INVOKABLE void loadContractIntoTemp(int companyId, int contractId);

    // Form reset handling (triggered by lifecycle events)
    Q_INVOKABLE void resetFormForManualReset();                                         // Reset form when user manually clicks reset button
    Q_INVOKABLE void resetFormForCompanyChange();                                       // Reset form when company selection changes (triggered by user)
    // Q_INVOKABLE void resetFormForNewContractAfterSave(bool afterRevenue = false);       // Reset form after successful save to prepare for a new entry

    // Database and cache management
    void initializeCache();
    void loadCompanyData();                                                         // Fetch essential company data (ID, name) from the database and update cache
    void loadContracts();                                                           // Fetch all contracts for all companies from the database and update cache
    Q_INVOKABLE void loadContractsForCompany(int companyId);                        // Fetch all contracts for a single company from the database and update cache
    Q_INVOKABLE QVariantList loadRevenue(int companyId);                            // Load revenue for all contracts
    Q_INVOKABLE QVariantList loadRevenueForContract(int companyId, int contractId); // Fetch revenue for a specific contract

    // Data retrieval (using cache)
    QMap<int, CompanyData> getAllCompanyData() const;                                   // Retrieve the entire local cache
    CompanyData getCompanyDataById(int companyId) const;                                // Retrieve all of a company's data using its ID from the cache
    Q_INVOKABLE QList<int> getCompanyIds() const;                                       // Get all of the company IDs
    Q_INVOKABLE QStringList getCompanyNamesList() const;                                // Get the list of current company names
    Q_INVOKABLE int getCompanyIdByName(const QString &name) const;                      // Get a company's ID using its name
    Q_INVOKABLE QString getCompanyNameById(int companyId) const;                        // Get a company's name using its ID
    Q_INVOKABLE QList<ContractCondition> getContractsByCompanyId(int companyId) const;  // Get all of the contracts for a company
    Q_INVOKABLE QVariantList getContractsAsVariantListByCompanyId(int companyId) const; // Get the contracts as QVariantList for QML

    // State variable management
    Q_INVOKABLE int selectedCompanyId() const;                                      // Get currently selected company ID
    Q_INVOKABLE void setSelectedCompanyId(int id);                                  // Set selected company ID (e.g. after dropdown change)
    Q_INVOKABLE void toggleContractSelection(int contractId);                       // Select/deselect a contract (for multi-selection)
    Q_INVOKABLE int getSingleSelectedContract() const;                              // Get currently selected contract (if exactly one is selected)
    Q_INVOKABLE QVariantList getSelectedContracts() const;                          // Get list of all selected contract IDs
    Q_INVOKABLE void clearSelectedContracts();                                      // Clear all contract selections (not including the last loaded contract)
    Q_INVOKABLE bool isContractSelected(int contractId) const;                      // Check if a contract is selected
    Q_INVOKABLE int selectMostRecentContract(int companyId);                       // Auto-select the most recently saved/added contract

    // Helpers
    Q_INVOKABLE bool canSaveContract(const QVariantMap &formState) const;
    Q_INVOKABLE bool canGenerateRevenue(const QVariantMap &formState) const;
    Q_INVOKABLE bool ensureContractSavedBeforeRevenue(int &contractId);
    Q_INVOKABLE bool validatePercentageString(const QString &percentages) const;    // Validate percentages format (for QML)
    Q_INVOKABLE bool validateCurrentForm() const;                                   // Validate all form fields inside tempContractEdit; returns true if valid, otherwise false and stores error message
    Q_INVOKABLE QString lastValidationError() const;                                // Get last form validation error (for QML display)
    // Q_INVOKABLE bool isGeneratingRevenue() const;
    // Q_INVOKABLE void setGeneratingRevenue(bool isGenerating);

    // Getters and setters for contract form fields (using cache)
    Q_INVOKABLE int getStartMonth(int companyId, int index) const;
    Q_INVOKABLE void setStartMonth(int companyId, int startMonth);
    Q_INVOKABLE int getStartYear(int companyId, int index) const;
    Q_INVOKABLE void setStartYear(int companyId, int startYear);
    Q_INVOKABLE double getInitialRevenue(int companyId, int index) const;
    Q_INVOKABLE void setInitialRevenue(int companyId, double initialRevenue);
    Q_INVOKABLE double getThreshold(int companyId, int index) const;
    Q_INVOKABLE void setThreshold(int companyId, double threshold);
    Q_INVOKABLE int getDelay(int companyId, int index) const;
    Q_INVOKABLE void setDelay(int companyId, int delay);
    Q_INVOKABLE bool getMultiPay() const;
    Q_INVOKABLE void setMultiPay(bool value);
    Q_INVOKABLE int getSecondDelay() const;
    Q_INVOKABLE void setSecondDelay(int value);
    Q_INVOKABLE QString getPercentages(int companyId, int index) const;
    Q_INVOKABLE void setPercentages(int companyId, const QString &percentages);

    // Revenue breakdown
    Q_INVOKABLE bool prepareAndGenerateRevenue(const QVariantMap &formData);
    Q_INVOKABLE void generateRevenue(int companyId, int contractId);
    Q_INVOKABLE QMap<int, QMap<int, double>> calculateRevenue(int companyId, int contractId);
    Q_INVOKABLE QVariantList getRevenueAsVariantList(int companyId, int contractId);
    Q_INVOKABLE void updateRevenueSummaryCache();
    Q_INVOKABLE QVariantList getYearlyRevenueSummary(int year);
    Q_INVOKABLE void saveCsvToFile(const QString &csvContent, const QString &filePath);

public slots:
    // Core data reload (settings and initialization)
    Q_INVOKABLE void reloadFromDatabase();                      // Reload all companies and contracts from database into cache (called at startup or after major data changes)

signals:
    // Core state signals
    void selectedCompanyIdChanged();                // Emitted whenever the selected company in the dropdown changes
    void selectedContractChanged();                 // Emitted whenever the selected contract in the table changes

    // Cache and contract lifecycle signals
    void companyCacheUpdated();                     // Emitted after any change to the **company cache** (add/edit/remove company)
    void contractChanged(int companyId);            // Emitted when a single contract (for a specific company) is added, edited, or removed
    void contractsReloaded();                       // Emitted after bulk reload of **all contracts** for a company (e.g. after company deletion)

    // Form lifecycle signals
    void formPopulated(int contractId);             // Emitted when an existing contract is loaded into the form (ready for editing)
    void formChanged();                             // Emitted when any **single field** changes (e.g. user typing in a field)
    void formReset();                               // Emitted when the entire form is cleared (after reset button or save)

    // Save and validation signals
    void contractSavedSuccessfully();               // Emitted after successful save (new or update)
    void contractUpdatedSuccessfully();

    // Revenue lifecycle signals
    void revenueGenerated(int contractId);          // Emitted after successfully generating revenue for a contract
    void isGeneratingRevenueChanged();              // Emitted to notify QML when revenue generation starts/stops

private slots:
    // Contract-level handlers
    void handleContractChanged(int companyId);                          // Internal handler when a single contract changes (wired to contractChanged signal)
    void handleContractsReloaded();                                     // Internal handler when all contracts are reloaded (wired to contractsReloaded signal)

    // Company-level handlers
    void handleCompanyAdded(int companyId, const QString &name);        // Internal handler when a new company is added
    void handleCompanyEdited(int companyId, const QString &newName);    // Internal handler when a company is renamed
    void handleCompanyRemoved(int companyId);                           // Internal handler when a company is removed

private:
    // Validation and utility functions (internal)
    bool isNameValid(const QString &name) const;                                        // Validate company/contract name (internal helper)
    bool validatePercentageList(const QList<double> &percentages, bool multiPay) const; // Validate percentages (used internally in saveContract)
    QList<double> parsePercentageStringToList(const QString &percentagesStr) const;     // Convert "40/60" string to {40.0, 60.0}
    QString formatPercentageListToString(const QList<double> &percentages) const;       // Convert {40.0, 60.0} back to "40/60"
    mutable QString m_lastValidationError;                                              // Store the last validation error message (for internal and QML)
    // bool m_isGeneratingRevenue = false;                                                 // Track revenue generation state

    // Internal members (state and storage)
    int m_selectedCompanyId = -1;                                           // Currently selected company ID (linked to dropdown)
    QList<int> m_selectedContractIds;                                       // Currently selected contract IDs (for multi-selection handling)
    QMap<int, CompanyData> m_companyDataCache;                              // Cache: company ID -> CompanyData (contract list, etc.)
    QVariantMap tempContractEdit;                                           // Temporary storage for form values during editing
    QHash<int, QMap<int, QMap<int, QMap<int, double>>>> m_revenueData;      // Revenue breakdown cache: Company ID -> { Contract ID -> { Year -> { Month -> Revenue } } }
    QHash<int, QMap<int, QMap<int, double>>> m_revenueSummary;              // Monthly revenue summary cache (for a selected year), including all companies
    CompanyListModel *m_companyModel = nullptr;                             // Pointer to CompanyListModel (external dependency)
    QSqlDatabase m_db = QSqlDatabase::database("AppConnection");            // Global database connection
};

#endif // CONTRACTMODEL_H
