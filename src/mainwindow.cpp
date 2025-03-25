#include "mainwindow.h"
#include <QQuickWidget>
#include <QVBoxLayout>
#include <QQmlContext>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    // Create a QQuickWidget to load QML content
    QQuickWidget *quickWidget = new QQuickWidget(this);
    quickWidget->setSource(QUrl(QStringLiteral("qrc:/main.qml")));  // Load QML
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // Set the widget as the central widget of the QMainWindow
    setCentralWidget(quickWidget);
}

MainWindow::~MainWindow() {}
