QT       += core gui qml quick quickwidgets quickcontrols2 sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    CompanyListModel.cpp \
    ContractModel.cpp \
    DatabaseInitializer.cpp \
    DatabaseUtils.cpp \
    SettingsController.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    CompanyListModel.h \
    ContractModel.h \
    DatabaseInitializer.h \
    DatabaseUtils.h \
    QtSqlIncludes.h \
    SettingsController.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    FujitsubameFinance_ja_JP.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    CompaniesPage.qml \
    SettingsPage.qml \
    SettingsWarningDialog.qml \
    StatisticsPage.qml \
    StyledButton.qml \
    SumPage.qml \
    WarningDialog.qml \
    main.qml

RESOURCES += \
    resources.qrc
