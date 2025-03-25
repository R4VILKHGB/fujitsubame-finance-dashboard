import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: styledButton
    text: qsTr("Styled Button") // Default button text
    font.pixelSize: 14        // Default font size
    highlighted: true

    Material.background: Material.accent
    Material.foreground: "Black"
}
