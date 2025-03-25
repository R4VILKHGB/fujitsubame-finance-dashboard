import QtQuick 2.15
import QtQuick.Controls 6.8
import QtQuick.Layouts 1.15
import Qt.labs.qmlmodels
import QtQml
import QtQuick.Dialogs
import "qrc://StyledButton.qml"
import "qrc://WarningDialog.qml"

Rectangle {
    id: sumPage
    anchors.fill: parent

    property int selectedYear: 0 // Empty or uninitialized state
    property var columnWidths: [100, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76]
    property int columnTotalWidth: columnWidths.reduce((a, b) => a + b, 0)

    TableModel {
        id: summaryModel

        TableModelColumn { display: "company" }
        TableModelColumn { display: "month1" }
        TableModelColumn { display: "month2" }
        TableModelColumn { display: "month3" }
        TableModelColumn { display: "month4" }
        TableModelColumn { display: "month5" }
        TableModelColumn { display: "month6" }
        TableModelColumn { display: "month7" }
        TableModelColumn { display: "month8" }
        TableModelColumn { display: "month9" }
        TableModelColumn { display: "month10" }
        TableModelColumn { display: "month11" }
        TableModelColumn { display: "month12" }
    }

    // Scrollable page
    ScrollView {
        id: mainScrollView
        anchors.fill: parent
        // contentWidth: parent.width
        // contentWidth: mainLayout.width
        contentWidth: Math.max(mainLayout.implicitWidth, sumPage.columnTotalWidth + 140)
        // contentHeight: parent.height
        clip: true

        Item {
            id: contentWrapper
            width: sumPage.columnTotalWidth + 140 // total content width + margins
            height: childrenRect.height

            // Main layout
            ColumnLayout {
                id: mainLayout
                // anchors.fill: parent
                // width: Math.max(sumPage.columnTotalWidth, mainScrollView.width)
                width: sumPage.columnTotalWidth
                x: 70 // Left margin
                // anchors.top: parent.top
                // anchors.left: parent.left
                // anchors.leftMargin: 70
                // anchors.rightMargin: 70
                spacing: 10

                // Top margin
                Item {
                    Layout.preferredHeight: 10
                }

                // Header
                Text {
                    text: qsTr("Yearly Revenue Summary")
                    font.pixelSize: 20
                    font.bold: true
                    color: Material.foreground
                    Layout.alignment: Qt.AlignLeft
                }

                // Year selection
                Frame {
                    // Layout.fillWidth: true
                    Layout.preferredWidth: sumPage.columnTotalWidth
                    padding: 10

                    background: Rectangle {
                        color: "transparent"
                        radius: Material.LargeScale
                        // border.color: Material.accent
                        // border.width: 1
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 15
                        width: sumPage.columnTotalWidth

                        Text {
                            text: qsTr("Enter Year:")
                            font.pixelSize: 14
                            color: Material.foreground
                        }

                        TextField {
                            id: yearInput
                            // text: sumPage.selectedYear.toString()
                            text: sumPage.selectedYear > 0 ? sumPage.selectedYear.toString() : ""
                            Layout.preferredWidth: 100
                            font.pixelSize: 14
                            color: Material.foreground
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            validator: IntValidator { bottom: 1 }

                            background: Rectangle {
                                radius: Material.LargeScale
                                color: "transparent"
                                border.color: yearInput.acceptableInput ? Material.accent : "red"
                                border.width: 1
                            }

                            onEditingFinished: {
                                let year = parseInt(text);
                                if (year < 1 || isNaN(year)) {
                                    globalWarningDialog.message = qsTr("Year must be greater than 0.");
                                    globalWarningDialog.open();
                                    yearInput.text = sumPage.selectedYear.toString();
                                } else {
                                    sumPage.selectedYear = year;
                                    loadSummaryData();
                                }
                            }
                        }

                        // Inline validation message
                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: yearInputError.implicitHeight

                            Label {
                                id: yearInputError
                                text: yearInput.text.length > 0 && (!yearInput.acceptableInput || parseInt(yearInput.text) <= 0)
                                      ? qsTr("Please enter a valid year (e.g. 2024).")
                                      : " " // Non-breaking space to preserve height
                                color: "red"
                                font.pixelSize: 12
                                anchors.left: parent.left
                                anchors.right: parent.right
                                elide: Text.ElideRight
                                maximumLineCount: 1
                            }
                        }

                        StyledButton {
                            id: loadDataButton
                            text: qsTr("Load Data")
                            // onClicked: loadSummaryData()
                            onClicked: {
                                let year = parseInt(yearInput.text);
                                if (isNaN(year) || year <= 0) {
                                    globalWarningDialog.message = qsTr("Please enter a valid year greater than 0.");
                                    globalWarningDialog.open();
                                } else {
                                    sumPage.selectedYear = year;
                                    sumPage.loadSummaryData();
                                }
                            }
                        }

                        // Spacer
                        Item {
                            Layout.fillWidth: true
                        }

                        // Export
                        StyledButton {
                            id: exportCsvButton
                            text: qsTr("Export to CSV")
                            Layout.alignment: Qt.AlignRight

                            onClicked: {
                                fileSaveDialog.open()
                                console.log("Exporting to CSV done...");
                            }
                        }

                        // Reset
                        StyledButton {
                            id: resetButton
                            text: qsTr("Reset")
                            Layout.alignment: Qt.AlignRight

                            onClicked: {
                                console.log("Resetting data...");
                                // Reset year
                                sumPage.selectedYear = 0;
                                yearInput.text = "";

                                // Clear existing data
                                summaryModel.clear();
                            }
                        }
                    }
                }

                // Revenue table
                Frame {
                    // Layout.fillWidth: true
                    // Layout.preferredHeight: parent.height * 0.7
                    Layout.preferredWidth: sumPage.columnTotalWidth
                    Layout.preferredHeight: implicitHeight
                    Layout.fillHeight: false
                    padding: 0

                    background: Rectangle {
                        radius: Material.LargeScale
                        color: "transparent"
                        // border.color: Material.accent
                        // border.width: 1
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 0

                        // Table header row
                        Row {
                            id: monthHeader
                            // Layout.fillWidth: true
                            Layout.preferredWidth: sumPage.columnTotalWidth
                            Layout.preferredHeight: 40
                            spacing: 2

                            Repeater {
                                model: 13

                                delegate: Rectangle {
                                    width: sumPage.columnWidths[index]
                                    height: 40
                                    // implicitWidth: 80
                                    // implicitHeight: 40
                                    color: Material.accent

                                    Text {
                                        anchors.centerIn: parent
                                        font.bold: true
                                        font.pixelSize: 14
                                        color: Material.foreground
                                        text: index === 0 ? qsTr("Company") : index.toString()
                                    }
                                }
                            }
                        }

                        // Main table body
                        TableView {
                            id: revenueTable
                            // Layout.fillWidth: true
                            Layout.preferredWidth: sumPage.columnTotalWidth
                            Layout.fillWidth: false
                            // Layout.fillHeight: true
                            implicitHeight: summaryModel.rowCount * 42 // 40 row height + 2 spacing
                            Layout.fillHeight: false
                            columnSpacing: 2
                            rowSpacing: 2
                            clip: false
                            interactive: false

                            model: summaryModel

                            columnWidthProvider: function(column) {
                                return sumPage.columnWidths[column];
                            }

                            delegate: Item {
                                implicitWidth: 80
                                implicitHeight: 40 + (row === 0 && column === 0 ? 2 : 0)

                                Column {
                                    anchors.fill: parent
                                    spacing: 0

                                    // Transparent gap only for the first company row
                                    Rectangle {
                                        visible: row === 0 && column === 0
                                        height: 2
                                        width: parent.width
                                        color: "transparent"
                                    }

                                    Rectangle {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        width: parent.width
                                        height: 40
                                        color: column === 0 ? Material.accent : "transparent"

                                        Text {
                                            anchors.centerIn: parent
                                            text: display
                                            font.pixelSize: 14
                                            font.bold: column === 0
                                            color: Material.foreground
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Warning dialog
    WarningDialog {
        id: globalWarningDialog
    }

    FileDialog {
        id: fileSaveDialog
        title: qsTr("Save CSV File")
        fileMode: FileDialog.SaveFile
        nameFilters: ["CSV files (*.csv)"]
        defaultSuffix: "csv"

        onAccepted: {
            if (selectedFile && selectedFile.toString() !== "") {
                console.log("Saving to:", selectedFile.toString());
                exportModelToCsv(summaryModel, selectedFile.toString());
            } else {
                console.warn("Selected file is empty.");
            }
        }

        onRejected: {
            console.log("User canceled export.");
        }
    }

    function loadSummaryData() {
        console.log("Fetching summary for year:", sumPage.selectedYear);

        if (typeof contractModel === "undefined" || !contractModel.getYearlyRevenueSummary) {
            console.error("contractModel is not available.");
            globalWarningDialog.message = qsTr("Data source is unavailable.");
            globalWarningDialog.open();
            return;
        }

        let summaryList = contractModel.getYearlyRevenueSummary(sumPage.selectedYear) || [];
        summaryModel.clear();

        if (summaryList.length === 0) {
            console.warn("No revenue summary found.");
            globalWarningDialog.message = qsTr("No revenue summary available for the selected year.");
            globalWarningDialog.open();
            return;
        }

        for (let i = 0; i < summaryList.length; i++) {
            let companyData = summaryList[i];

            if (!companyData || !companyData.companyName) {
                console.warn("Skipping invalid company data:", companyData);
                continue;
            }

            let rowData = {
                company: companyData.companyName
            };

            for (let j = 0; j < 12; j++) {
                let revenue = companyData.monthlyRevenues[j] || 0;
                rowData["month" + (j + 1)] = revenue;
            }

            summaryModel.appendRow(rowData);
        }

        console.log("Yearly revenue summary loaded.");
    }

    function exportModelToCsv(model, fileUrl) {
        if (!model || model.rowCount === 0 || model.columnCount === 0) {
            globalWarningDialog.message = qsTr("No data to export.");
            globalWarningDialog.open();
            return;
        }

        const roles = [
                        "company", "month1", "month2", "month3", "month4", "month5",
                        "month6", "month7", "month8", "month9", "month10", "month11", "month12"
                    ];
        const headers = [
                          "Company", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
                      ];

        let csvLines = [];

        // Add CSV header
        csvLines.push(headers.join(","));

        // Iterate through each row
        for (let row = 0; row < model.rowCount; ++row) {
            let rowData = [];
            for (let i = 0; i < roles.length; ++i) {
                let cellData = model.rows[row][roles[i]];

                // Properly escape CSV content
                if (typeof cellData === "string") {
                    cellData = cellData.replace(/"/g, '""'); // Escape double quotes
                }

                // Ensure cellData is properly quoted
                rowData.push(`"${cellData}"`);
            }
            csvLines.push(rowData.join(","));
        }

        // Combine lines into a single CSV string
        const csvContent = csvLines.join("\n");

        // Backend call wrapped with error handling
        try {
            contractModel.saveCsvToFile(csvContent, fileUrl);
        } catch (error) {
            globalWarningDialog.message = qsTr("Export failed: ") + error;
            globalWarningDialog.open();
        }
    }


    function onRevenueGenerated(contractId) {
        if (!contractModel || !contractModel.getContractById)
            return;

        let companyId = contractModel.selectedCompanyId;
        let contract = contractModel.getContractById(companyId, contractId);

        let contractYear = contract.startYear;
        let selected = sumPage.selectedYear;

        console.log("Revenue generated for contract:", contractId, "Year:", contractYear, "Selected year:", selected);

        if (parseInt(contractYear) === parseInt(selected)) {
            console.log("Auto-refreshing summary for year:", selected);
            loadSummaryData();
        }
    }
}
