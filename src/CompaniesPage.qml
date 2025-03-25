import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc://StyledButton.qml"

Rectangle {
    id: companiesPage
    anchors.fill: parent

    // Reset filter string when the page is loaded or becomes visible
    Component.onCompleted: {
        companyListModel.filterString = ""; // Clear the filter on initial load
    }
    onVisibleChanged: {
        if (visible) {
            companyListModel.filterString = ""; // Reset the filter when the page becomes visible
        }
    }

    // Scrollable page
    ScrollView {
        id: mainScrollView
        anchors.fill: parent
        contentWidth: parent.width
        clip: true

        // Main layout
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
                text: qsTr("Companies")
                font.pixelSize: 20
                font.bold: true
                color: Material.foreground
                Layout.alignment: Qt.AlignLeft
            }

            // Header row: search bar and add company button
            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle {
                    color: "transparent"
                }

                // Header row
                RowLayout {
                    width: parent.width
                    height: 60
                    spacing: 30

                    // Search bar
                    TextField {
                        id: searchBar
                        Layout.fillWidth: true
                        Layout.preferredHeight: addCompanyButton.height * 0.78
                        placeholderText: qsTr("Search companies...")
                        font.pixelSize: 16
                        Material.foreground: Material.foreground
                        background: Rectangle {
                            radius: Material.LargeScale
                            color: Material.background
                            border.color: Material.accent
                            border.width: 1
                        }
                        onTextChanged: {
                            companyListModel.filterString = text; // No need to trim here
                            console.log("Filter string updated to:", companyListModel.filterString);
                        }
                    }

                    // Add company button
                    StyledButton {
                        id: addCompanyButton
                        text: qsTr("Add Company")
                        onClicked: {
                            newName.text = "";
                            addCompanyDialog.open();
                        }
                    }
                }
            }

            // List of companies
            Repeater {
                model: companyListModel

                // Company card (each in a row)
                delegate: Rectangle {
                    Layout.fillWidth: true
                    height: 80
                    radius: Material.LargeScale // Rounded corners
                    color: Material.background // Background color
                    border.color: Material.accent // Accent color for the border
                    border.width: 1

                    Layout.topMargin: 10

                    // Card content
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 20

                        // Company name
                        Text {
                            text: model.name.length > 0 ? model.name : qsTr("Unnamed Company") // text: name || "Unnamed Company" // Fallback for empty names
                            font.pixelSize: 16
                            font.bold: true
                            color: "Black"
                            elide: Text.ElideRight // Truncate text if it's too long
                            Layout.fillWidth: true
                        }

                        // Buttons: edit, delete
                        RowLayout {
                            spacing: 10
                            Layout.alignment: Qt.AlignRight

                            // Edit
                            StyledButton {
                                text: qsTr("Edit")
                                Material.background: Material.Amber
                                Material.foreground: "White"
                                Layout.preferredWidth: 90
                                onClicked: {
                                    if (model.id) {
                                        editCompanyDialog.editCompanyId = model.id; // Set the company ID
                                        editCompanyDialog.originalName = model.name; // Pre-fill name field
                                        editCompanyDialog.open(); // Open the edit dialog
                                    } else {
                                        console.error("Edit failed: Invalid company ID");
                                    }
                                }
                            }

                            // Delete
                            StyledButton {
                                text: qsTr("Delete")
                                Material.background: Material.DeepOrange
                                Material.foreground: "White"
                                Layout.preferredWidth: 90
                                onClicked: {
                                    if (model.id) {
                                        companyListModel.removeCompany(model.id); // Remove the company
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Bottom margin
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 10
            }
        }

        // Add company dialog
        Dialog {
            id: addCompanyDialog
            modal: true
            title: qsTr("Add Company")
            width: parent.width * 0.5
            height: parent.height * 0.4
            anchors.centerIn: parent
            margins: 20

            property bool isDuplicateName: false

            // Reset the text field and duplicate check when the dialog opens
            onOpened: {
                console.log("Add Company Dialog opened.");
                newName.text = ""; // Clear the input field
                isDuplicateName = false;
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                Layout.margins: 20
                Layout.fillHeight: true

                // Input field for company name
                TextField {
                    id: newName
                    placeholderText: qsTr("Enter Company Name")
                    font.pixelSize: 14
                    Layout.fillWidth: true
                    onTextChanged: {
                        const trimmedText = text.trim();
                        addCompanyDialog.isDuplicateName = companyListModel.getCompanyIdByName(trimmedText) >= 0;
                    }
                }

                // Duplicate warning label
                Label {
                    visible: addCompanyDialog.isDuplicateName
                    text: qsTr("A company with this name already exists. Please choose a different name.")
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "red"
                    font.pixelSize: 14
                    wrapMode: Text.Wrap
                }

                // Spacer
                Item {
                    Layout.fillWidth: true
                }

                // Buttons
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    // Save
                    StyledButton {
                        text: qsTr("Save")
                        enabled: !addCompanyDialog.isDuplicateName && newName.text.trim() !== "" // Button is enabled only if the name is non-empty and not a duplicate
                        onClicked: {
                            const trimmedName = newName.text.trim();
                            const success = companyListModel.addCompany(trimmedName);
                            if (success) {
                                console.log("Company added successfully:", trimmedName);
                                addCompanyDialog.close(); // Close the dialog
                            } else {
                                console.error("Failed to add company:", trimmedName);
                            }
                        }
                    }

                    // Cancel
                    StyledButton {
                        text: qsTr("Cancel")
                        onClicked: {
                            newName.text = ""; // Clear the input field
                            addCompanyDialog.close();
                        }
                    }
                }
            }
        }

        // Edit company dialog
        Dialog {
            id: editCompanyDialog
            modal: true
            title: qsTr("Edit Company")
            width: parent.width * 0.5
            height: parent.height * 0.4
            anchors.centerIn: parent
            margins: 20

            property int editCompanyId: -1
            property string originalName: ""
            property bool isDuplicateName: false

            // Reset the text field and duplicate check when the dialog opens
            onOpened: {
                console.log("Dialog opened with originalName:", originalName);
                editNameField.text = originalName;
                isDuplicateName = false;
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                Layout.margins: 20
                Layout.fillHeight: true

                // Input field for the new name
                TextField {
                    id: editNameField
                    text: editCompanyDialog.originalName // Pre-fill the field with the company's current name
                    placeholderText: qsTr("Edit Company Name")
                    font.pixelSize: 14
                    Layout.fillWidth: true
                    onTextChanged: {
                        const trimmedText = text.trim();
                        if (trimmedText === editCompanyDialog.originalName) {
                            editCompanyDialog.isDuplicateName = false;
                        } else {
                            editCompanyDialog.isDuplicateName = companyListModel.getCompanyIdByName(trimmedText) >= 0;
                        }
                    }
                }

                // Duplicate warning label
                Label {
                    visible: editCompanyDialog.isDuplicateName
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: qsTr("A company with this name already exists. Please choose a different name.")
                    color: "red"
                    font.pixelSize: 14
                    wrapMode: Text.Wrap
                }

                // Spacer
                Item {
                    Layout.fillWidth: true
                }

                // Buttons
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    // Save
                    Button {
                        text: qsTr("Save")
                        enabled: !editCompanyDialog.isDuplicateName
                                 && editNameField.text.trim() !== ""
                                 && editNameField.text.trim() !== editCompanyDialog.originalName
                        onClicked: {
                            const newName = editNameField.text.trim();
                            const success = companyListModel.editCompany(editCompanyDialog.editCompanyId, newName);
                            if (success) {
                                console.log("Company edited successfully. ID:", editCompanyDialog.editCompanyId, "New Name:", newName);
                                editNameField.text = "";
                                editCompanyDialog.close();
                            } else {
                                console.error("Failed to edit company:", newName);
                                errorLabel.text = qsTr("Failed to edit company. Please try again.");
                                errorLabel.visible = true;
                            }
                        }
                    }

                    // Cancel
                    Button {
                        text: qsTr("Cancel")
                        onClicked: {
                            editNameField.text = editCompanyDialog.originalName; // Reset to original
                            editCompanyDialog.close(); // Close the dialog
                        }
                    }
                }
            }
        }
    }
}
