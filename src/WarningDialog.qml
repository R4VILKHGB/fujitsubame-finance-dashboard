import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: warningDialog
    modal: true
    width: parent.width * 0.5
    height: parent.height * 0.3
    anchors.centerIn: parent
    margins: 10

    ColumnLayout {
        anchors.fill: parent
        Layout.margins: 8
        spacing: 10

        Label {
            id: warningTextLabel
            text: warningDialog.message
            font.pixelSize: 14
            color: "red"
            horizontalAlignment: Qt.AlignHCenter
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            maximumLineCount: 30 // Prevent overflow
            elide: Text.ElideRight // Cut off text with "..." if needed
        }

        Button {
            text: qsTr("OK")
            Layout.alignment: Qt.AlignHCenter
            onClicked: warningDialog.close()
        }
    }

    // Custom property to set the warning message dynamically
    property string message: ""
}
