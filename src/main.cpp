#include "DatabaseInitializer.h"
#include "CompanyListModel.h"
#include "ContractModel.h"
#include "SettingsController.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QtQuickControls2/QtQuickControls2>
#include <QDebug>
#include <QTranslator>
#include <QLocale>
#include <QDir>
#include <QQuickStyle>

int main(int argc, char *argv[]) {
    qDebug() << "Starting FujitsubameFinance...";
    QGuiApplication app(argc, argv);

    // Set the Material design style
    QQuickStyle::setStyle("Material");

    // Load Japanese translation
    QTranslator translator;
    QString translationFile = "FujitsubameFinance_ja_JP.qm";

    qDebug() << "Current working directory:" << QDir::currentPath();

    // Try loading from current directory
    if (translator.load(translationFile, QDir::currentPath())) {
        app.installTranslator(&translator);
        qDebug() << "Japanese translation loaded from:" << QDir::currentPath() + "/" + translationFile;
    } else {
        qWarning() << "Failed to load translation:" << translationFile;
    }

    // Initialize the database and get the path
    QString dbPath = initializeDatabase();
    if (dbPath.isEmpty()) {
        qWarning() << "Database initialization failed!";
        return -1;
    }

    // Check if the database file exists
    // QFile dbFile(dbPath);
    // if (!dbFile.exists()) {
    //     qCritical() << "Database file does not exist at:" << dbPath;
    //     return -1;
    // } else {
    //     qDebug() << "Database file already exists at:" << dbPath;
    // }

    // Debug statements
    qDebug() << "Database connection opened successfully at:" << dbPath;

    // Open a persisten database connection
    // QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "AppConnection");
    // db.setDatabaseName(dbPath);
    // db.open(); // SQLite operates on local database files. SQlite doesn't have the concept of a database server.

    // Explicitly store the initial database path if not already set
    QSettings settings("Fujitsubame", "FinanceDashboard");
    if (settings.value("databasePath").toString().isEmpty()) {
        settings.setValue("databasePath", dbPath);
        qDebug() << "Initial database path saved to settings:" << dbPath;
    }

    // Initialize SettingsController and connect to database
    SettingsController settingsController;
    if (!settingsController.connectToDatabase(settingsController.databasePath())) {
        qCritical() << "Failed to connect to database during startup.";
        return -1;
    }

    // Create instances of core models after database is connected
    CompanyListModel companyListModel;
    QSqlDatabase db = settingsController.getDatabaseConnection();
    ContractModel contractModel(&companyListModel, db);

    // Connect settings signals to model slots for synchronization
    QObject::connect(&settingsController, &SettingsController::databasePathChanged, [&]() {
        qDebug() << "Database path changed to:" << settingsController.databasePath();
    });

    QObject::connect(&settingsController, &SettingsController::dataRefreshed, &companyListModel, &CompanyListModel::reloadFromDatabase);
    QObject::connect(&settingsController, &SettingsController::dataRefreshed, &contractModel, &ContractModel::reloadFromDatabase);
    QObject::connect(&settingsController, &SettingsController::dataCleared, &companyListModel, &CompanyListModel::reloadFromDatabase);
    QObject::connect(&settingsController, &SettingsController::dataCleared, &contractModel, &ContractModel::reloadFromDatabase);

    // Initialize the QML application engine
    QQmlApplicationEngine engine;

    // Expose the models to QML
    engine.rootContext()->setContextProperty("companyListModel", &companyListModel);
    engine.rootContext()->setContextProperty("contractModel", &contractModel);
    engine.rootContext()->setContextProperty("settingsController", &settingsController);

    // Load the main QML file
    const QUrl url(QStringLiteral("qrc:/main.qml")); // Path to the QML file
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                     [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);

    engine.load(url);

    qDebug() << "Application started successfully.";

    // Execute the application
    return app.exec();
}
