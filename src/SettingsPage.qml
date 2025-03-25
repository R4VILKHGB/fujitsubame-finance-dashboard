import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import Qt.labs.platform as Labs
import QtQml
import "qrc://SettingsWarningDialog.qml"
import "qrc://StyledButton.qml"

Rectangle {
    id: settingsPage
    anchors.fill: parent

    // Scrollable page
    ScrollView {
        id: mainScrollView
        anchors.fill: parent
        contentWidth: parent.width
        clip: true

        // Main layout: vertical stacking of sections
        ColumnLayout {
            id: mainLayout
            anchors.fill: parent
            anchors.leftMargin: 70
            anchors.rightMargin: 70
            spacing: 10

            // Top margin
            Item {
                Layout.preferredHeight: 10
            }

            // Page header
            Text {
                text: qsTr("Settings")
                font.pixelSize: 20
                font.bold: true
                color: Material.foreground
                Layout.alignment: Qt.AlignLeft
            }

            // Database management section
            GroupBox {
                Layout.fillWidth: true
                padding: 8
                background: Rectangle {
                    radius: Material.LargeScale
                    color: Material.background
                    border.color: Material.accent
                    border.width: 1
                }

                ColumnLayout {
                    spacing: 10
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Current Database Location")
                        font.pixelSize: 14
                    }

                    RowLayout {
                        spacing: 10
                        Layout.fillWidth: true

                        TextField {
                            id: dbPathField
                            text: settingsController.databasePath
                            readOnly: true
                            Layout.fillWidth: true
                            background: Rectangle {
                                radius: Material.LargeScale
                                color: "transparent"
                                border.color: Material.accent
                                border.width: 1
                            }
                        }

                        StyledButton {
                            text: qsTr("Change Database Location")
                            onClicked: fileDialog.open()

                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Select a different database file or location.")
                        }
                    }
                }
            }

            // Data operations
            GroupBox {
                Layout.fillWidth: true
                padding: 8
                background: Rectangle {
                    radius: Material.LargeScale
                    color: Material.background
                    border.color: Material.accent
                    border.width: 1
                }

                RowLayout {
                    spacing: 10
                    Layout.fillWidth: true

                    // Refresh data
                    StyledButton {
                        text: qsTr("Refresh Data")
                        onClicked: settingsController.refreshAllData()
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Reloads data from the database. Use if data seems incorrect.")
                    }

                    StyledButton {
                        text: qsTr("Clear All Data")
                        highlighted: true

                        onClicked: {
                            settingsWarningDialog.message = qsTr("Are you sure you want to clear ALL data? This action CANNOT be undone.")
                            settingsWarningDialog.open()
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Deletes all data permanently. Cannot be undone!")
                    }
                }
            }

            // Danger zone
            // GroupBox {
            //     Layout.fillWidth: true
            //     padding: 8
            //     background: Rectangle {
            //         radius: Material.LargeScale
            //         border.width: 1
            //     }

            //     RowLayout {
            //         spacing: 10
            //         Layout.fillWidth: true

            //         // Danger icon
            //         // Rectangle {
            //         //     Layout.preferredWidth: 80
            //         //     Layout.preferredHeight: 80
            //         //     color: "transparent"

            //         //     Image {
            //         //         source: "qrc:/icons/circle-exclamation-solid.svg"
            //         //         width: parent.width * 0.55
            //         //         height: parent.height * 0.55
            //         //         fillMode: Image.PreserveAspectFit
            //         //         smooth: true
            //         //         anchors.centerIn: parent
            //         //     }
            //         // }
            //     }
            // }

            // App information
            GroupBox {
                Layout.fillWidth: true
                padding: 12
                background: Rectangle {
                    radius: Material.LargeScale
                    color: Material.background
                    border.color: Material.accent
                    border.width: 1
                }

                ColumnLayout {
                    spacing: 10
                    width: parent.width - 24  // Important for wrapping within GroupBox (12 padding left + 12 padding right)

                    Label {
                        text: qsTr("App Version: 1.0.0")
                        font.bold: true
                        font.pixelSize: 16
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("This application simplifies the management of company revenues and contracts. With this app, you can easily:")
                        wrapMode: Text.WordWrap
                        font.pixelSize: 14
                        Layout.fillWidth: true
                    }

                    ColumnLayout {
                        spacing: 8
                        Layout.fillWidth: true
                        Layout.leftMargin: 12

                        Label {
                            text: qsTr("• Manage Companies: Add new companies, or edit and remove existing ones using the Companies page.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 14
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("• Manage Contracts: Create and manage contracts on the Form page. Specify initial revenue amounts, payment schedules, and thresholds. Generate and save revenue data, manage existing contracts, and access detailed revenue breakdowns.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 14
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("• View Revenue Summaries: Easily review a month-by-month summary of each company's revenue for any selected year on the Revenue Summary page.\n")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 14
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Security Notice: All data is stored locally on your device. This app does not connect to the internet or store data in the cloud.\nTo back up your data, locate your database file (usually saved in the AppData folder or your chosen location), and make a copy of it by right-clicking → Copy, then paste it into a safe folder (e.g. USB drive or cloud folder).\nTo restore a backup, simply use the 'Change Database Location' button and select your saved copy.")
                            wrapMode: Text.WordWrap
                            font.pixelSize: 14
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // Status message display
            Label {
                id: resultLabel
                text: ""
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                color: "darkred"
                visible: text.length > 0
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
            }

            // Timer to clear status messages automatically
            Timer {
                id: clearMessageTimer
                interval: 4000
                repeat: false
                onTriggered: resultLabel.text = ""
            }

            // Save
            // StyledButton {
            //     text: "Save"
            //     Layout.alignment: Qt.AlignRight
            //     onClicked: {
            //         clearStatusMessage()
            //         resultLabel.text = "Settings saved successfully"
            //         resultLabel.color = "green"
            //     }
            // }

            // Bottom margin
            // Item {
            //     Layout.fillWidth: true
            //     Layout.preferredHeight: 10
            // }
        }
    }

    SettingsWarningDialog {
        id: settingsWarningDialog
        onConfirmed: {
            settingsController.clearAllData()
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Select Database File")
        fileMode: FileDialog.OpenFile
        currentFolder: Labs.StandardPaths.writableLocation(Labs.StandardPaths.AppDataLocation)
        nameFilters: ["SQLite Database (*.db)"]
        onAccepted: settingsController.setDatabasePath(fileDialog.selectedFile.toString())
    }

    // Confirmation dialog for clearing data
    Dialog {
        id: confirmDialog
        title: qsTr("Clear All Data")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        Label {
            text: qsTr("Are you sure you want to clear ALL data? This action CANNOT be undone.")
            font.pixelSize: 14
            color: "red"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            padding: 10
            horizontalAlignment: Text.AlignHCenter
        }

        onAccepted: {
            settingsController.clearAllData()
        }
    }

    // Backend event listeners
    Connections {
        target: settingsController

        function onDatabasePathChanged() {
            // dbPathField.text = settingsController.databasePath
            resultLabel.text = qsTr("✅ Database path changed successfully.")
            resultLabel.color = "green"
            clearMessageTimer.restart()
        }

        function onDataCleared() {
            resultLabel.text = qsTr("✅ All data cleared successfully.")
            resultLabel.color = "green"
            clearMessageTimer.restart()
        }

        function onDataRefreshed() {
            resultLabel.text = qsTr("✅ Data refreshed successfully.")
            resultLabel.color = "green"
            clearMessageTimer.restart()
        }

        function onErrorOccurred(errorMessage) {
            resultLabel.text = qsTr("❌ Error: " + errorMessage)
            resultLabel.color = "darkred"
            clearMessageTimer.restart()
        }
    }
}
