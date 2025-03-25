import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc://StyledButton.qml"

ApplicationWindow {
    visible: true
    width: 1280
    height: 720
    title: qsTr("Fujitsubame Finance Dashboard")

    Material.theme: Material.Light
    Material.accent: "#d6d7d7"
    Material.background: "White"
    Material.foreground: "Black" // #2e2e3e

    property int selectedCompanyId: -1

    Rectangle {
        id: root
        anchors.fill: parent
        color: Material.background  // Main background color

        // Default starting page
        property string currentPage: "Companies"  // Tracks the active page

        // Sidebar
        Rectangle {
            id: sidebar
            width: 100
            color: "#84659d"  // Sidebar background color
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            z: 2

            // Sidebar buttons (companies, statistics, settings)
            Column {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: parent.height * 0.1  // Start 70% from the bottom
                spacing: 40

                // Companies button
                Rectangle {
                    width: 80
                    height: 80
                    radius: 40
                    color: "transparent"

                    Image {
                        source: "qrc:/icons/building-solid.svg"
                        width: parent.width * 0.5
                        height: parent.height * 0.5
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.currentPage = "Companies"
                        onEntered: parent.color = "#c9c0e1"  // Hover effect
                        onExited: parent.color = "transparent"
                    }
                }

                // Statistics button
                Rectangle {
                    width: 80
                    height: 80
                    radius: 40
                    color: "transparent"

                    Image {
                        source: "qrc:/icons/file-solid.svg"
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.currentPage = "Statistics"
                        onEntered: parent.color = "#c9c0e1"  // Hover effect
                        onExited: parent.color = "transparent"
                        // onPressed: parent.color = "#574867"  // Active/pressed color
                    }
                }

                // Sum button
                Rectangle {
                    width: 80
                    height: 80
                    radius: 40
                    color: "transparent"

                    Image {
                        source: "qrc:/icons/chart-simple-solid.svg"
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.currentPage = "Sum"
                        onEntered: parent.color = "#c9c0e1"  // Hover effect
                        onExited: parent.color = "transparent"
                        // onPressed: parent.color = "#574867"  // Active/pressed color
                    }
                }

                // Settings button
                Rectangle {
                    width: 80
                    height: 80
                    radius: 40
                    color: "transparent"

                    Image {
                        source: "qrc:/icons/gear-solid.svg"
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.currentPage = "Settings"
                        onEntered: parent.color = "#c9c0e1"  // Hover effect
                        onExited: parent.color = "transparent"
                    }
                }
            }
        }

        // Main content area
        Rectangle {
            id: mainContent
            anchors.fill: parent
            anchors.leftMargin: sidebar.width
            color: "#f4f3fa"  // Updated background color for main content
            z: 1

            Loader {
                id: pageLoader
                anchors.fill: parent
                source: {
                    if (root.currentPage === "Companies") Qt.resolvedUrl("CompaniesPage.qml")
                    else if (root.currentPage === "Statistics") Qt.resolvedUrl("StatisticsPage.qml")
                    else if (root.currentPage === "Settings") Qt.resolvedUrl("SettingsPage.qml")
                    else if (root.currentPage === "Sum") Qt.resolvedUrl("SumPage.qml")
                }
            }
        }
    }
}
