import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: settingsWarningDialog
    modal: true
    width: parent.width * 0.4
    height: parent.height * 0.25
    anchors.centerIn: parent
    margins: 10

    signal confirmed() // Add this signal to handle confirmation

    ColumnLayout {
        anchors.fill: parent
        Layout.margins: 8
        spacing: 10

        Label {
            id: settingsWarningTextLabel
            text: settingsWarningDialog.message
            font.pixelSize: 14
            color: "red"
            horizontalAlignment: Qt.AlignHCenter
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            maximumLineCount: 30
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10

            Button {
                text: qsTr("Cancel")
                font.pixelSize: 14        // Default font size
                highlighted: true
                Material.background: Material.accent
                Material.foreground: "Black"
                onClicked: settingsWarningDialog.close()
            }

            Button {
                text: qsTr("Confirm")
                font.pixelSize: 14        // Default font size
                highlighted: true
                Material.background: Material.accent
                Material.foreground: "Black"

                onClicked: {
                    settingsWarningDialog.confirmed()
                    settingsWarningDialog.close()
                }
            }
        }
    }

    property string message: ""
}
