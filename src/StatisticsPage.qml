import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml
import "qrc://StyledButton.qml"
import "qrc://WarningDialog.qml"

Rectangle {
    id: statisticsPage
    anchors.fill: parent

    // Central form state, local (the source of truth for UI)
    property var formState: ({
                                 startMonth: null,
                                 startYear: "",
                                 initialRevenue: null,
                                 threshold: null,
                                 delay: null,
                                 secondDelay: null,
                                 percentages: "",
                                 multiPay: false
                                 // formula: ""
                             })

    // Collect form data for backend
    function collectFormData() { // Collect form field values
        return {
            startMonth: formState.startMonth,
            startYear: parseInt(formState.startYear) || 0,
            initialRevenue: parseFloat(formState.initialRevenue) || 0,
            threshold: parseFloat(formState.threshold) || 0,
            delay: parseInt(formState.delay) || 0,
            secondDelay: parseInt(formState.secondDelay) || 0,
            percentages: formState.percentages.trim(),
            multiPay: formState.multiPay
        };
    }

    // Reset button enablement
    property bool formHasContent: false // Track if form contains any content
    function refreshFormHasContent() { // Help to recompute form content state
        formHasContent = (
                    formState.startMonth !== null ||
                    formState.startYear.trim() !== "" ||
                    (formState.initialRevenue !== null && formState.initialRevenue > 0) ||
                    (formState.threshold !== null && formState.threshold > 0) ||
                    (formState.delay !== null && formState.delay > 0) ||
                    (formState.multiPay && formState.secondDelay !== null && formState.secondDelay > 0) || // Only check secondDelay if multiPay is enabled
                    formState.percentages.trim() !== ""
                    );
        console.log(`[UI] formHasContent recalculated: ${formHasContent} (current formState snapshot:`, JSON.stringify(formState, null, 2), ")");
    }

    //Generate revenue button enablement
    property bool canGenerateRevenue: false // Track if the generate revenue button should be enabled
    function refreshCanGenerateRevenue() { // Refresh generate revenue button enablement
        const formData = collectFormData();
        canGenerateRevenue = contractModel.canGenerateRevenue(formData);

        console.log("refreshCanGenerateRevenue: Updated canGenerateRevenue =", canGenerateRevenue);
    }

    // Save button enablement
    property bool canSave: false // Track if the save button should be enabled (combination of completeness and change detection)
    function refreshCanSave() { // Refresh save button enablement
        const formData = collectFormData();
        canSave = contractModel.canProceedWithSave(formData);  // Backend owns final logic

        console.log("refreshCanSave: Updated canSave =", canSave);
    }

    // Backend event handlers
    Connections {
        target: contractModel

        // Requesting backend reset form when a new company is selected
        function onSelectedCompanyIdChanged() {
            console.log("Company changed, requesting backend form reset.");
            activeContractsPopup.lastLoadedContractId = -1;

            if (contractModel.selectedCompanyId >= 0) {
                contractModel.resetFormForCompanyChange();
            }

            // Ensure UI state reflects cleared form
            refreshFormHasContent();
            refreshCanSave();
            refreshCanGenerateRevenue();
        }

        // Triggered when backend emits formReset() signal, meaning:
        // - A manual reset was requested (resetFormForManualReset)
        // - A company change triggered a reset (resetFormForCompanyChange)
        // - A successful save triggered a reset (resetFormForNewContractAfterSave)
        function onFormReset() {
            console.log("formReset signal received...");

            activeContractsPopup.lastLoadedContractId = -1;

            // Clear the local formState to empty values
            statisticsPage.formState = {
                startMonth: null,
                startYear: "",
                initialRevenue: null,
                threshold: null,
                delay: null,
                secondDelay: null,
                percentages: "",
                multiPay: false
                // formula: ""
            };

            refreshUI(); // Refresh visible fields in the UI (e.g. text boxes, dropdowns)
            refreshFormHasContent(); // Recalculate formHasContent after full reset
            refreshCanGenerateRevenue();
            refreshCanSave();

            console.log("Form fields and state cleared successfully, ready for new input.");
        }

        // Populate form state when an existing contract is loaded for editing
        function onFormPopulated() {
            let temp = contractModel.getTempContract();
            console.log("formPopulated received, loading contract into formState", temp);

            statisticsPage.formState = {
                contractId: temp.contractId,
                companyId: temp.companyId,
                startMonth: temp.startMonth,
                startYear: temp.startYear.toString(),
                initialRevenue: temp.initialRevenue,
                threshold: temp.threshold,
                delay: temp.delay,
                secondDelay: temp.secondDelay,
                percentages: temp.percentages,
                multiPay: temp.multiPay
                // formula: temp.formula || ""
            };

            refreshUI();
            refreshFormHasContent();
            refreshCanGenerateRevenue();
            refreshCanSave();

            console.log("Form populated from loaded contract:", statisticsPage.formState);
        }

        function onContractSavedSuccessfully() {
            console.log("Contract saved successfully...");

            // Trigger auto revenue generation
            const contractId = contractModel.getCurrentContractId();
            const companyId = contractModel.selectedCompanyId;

            if (contractId > 0 && companyId > 0) {
                contractModel.generateRevenue(companyId, contractId);
                console.log("Revenue generation triggered automatically after save.");
            } else {
                console.warn("Cannot auto-generate revenue: invalid company or contract ID.");
            }


            // Important: Refresh UI logic to stay in sync with backend reset
            refreshFormHasContent();
            refreshCanSave();
            refreshCanGenerateRevenue();

            globalWarningDialog.message = qsTr("Contract saved successfully!");
            globalWarningDialog.open();

            // console.log("Resetting form after successful contract save.");
            // contractModel.resetFormForNewContractAfterSave(false); // Prepare for new contract input
        }

        function onContractUpdatedSuccessfully() {
            console.log("Contract updated successfully...");

            // Important: Refresh UI logic to stay in sync with backend reset
            refreshFormHasContent();
            refreshCanSave();
            refreshCanGenerateRevenue();

            globalWarningDialog.message = qsTr("Contract updated successfully.");
            globalWarningDialog.open();
        }
    }

    // Utility function to reflect formState into UI fields
    function refreshUI() {
        startMonthDropdown.currentIndex = formState.startMonth !== null ? formState.startMonth - 1 : -1;
        startYearField.text = formState.startYear;

        initialRevenueField.text = (formState.initialRevenue !== null)
                ? formState.initialRevenue.toFixed(2)
                : "";

        thresholdField.text = (formState.threshold !== null)
                ? formState.threshold.toFixed(2)
                : "";

        delayField.text = (formState.delay !== null)
                ? formState.delay.toString()
                : "";

        secondDelayField.text = (formState.secondDelay !== null)
                ? formState.secondDelay.toString()
                : "";

        multiPayCheckBox.checked = formState.multiPay; // Ensure checkbox state matches formState
        secondDelayField.visible = formState.multiPay;
        secondDelayLabel.visible = formState.multiPay;

        percentagesField.text = formState.percentages;

        // formulaInputField.text = formState.formula;

        // Clear inline validation messages
        startYearError.visible = false;
        initialRevenueError.visible = false;
        thresholdError.visible = false;
        delayError.visible = false;
        secondDelayError.visible = false;
        percentagesError.visible = false;
    }

    // Scrollable page
    ScrollView {
        id: mainScrollView
        anchors.fill: parent
        contentWidth: parent.width
        clip: true

        WarningDialog {
            id: globalWarningDialog
        }

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
                text: qsTr("Form")
                font.pixelSize: 20
                font.bold: true
                color: Material.foreground
                Layout.alignment: Qt.AlignLeft
            }

            // Company selection
            Frame {
                Layout.fillWidth: true
                padding: 0
                background: Rectangle {
                    color: "transparent"
                }

                // Company selection row elements
                RowLayout {
                    width: parent.width
                    spacing: 15

                    // Drop-down menu header
                    Label {
                        text: qsTr("Company Selection")
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }

                    // Company drop-down menu
                    ComboBox {
                        id: companyDropdown
                        Layout.preferredWidth: parent.width * 0.4
                        model: contractModel.getCompanyNamesList() // Mapping names and ids
                        currentIndex: -1 // Ensure the dropdown starts empty

                        // Ater initial load, populate list and restore selected company if applicable
                        Component.onCompleted: {
                            let companyIds = contractModel.getCompanyIds(); // Get all company IDs from cache
                            if (companyIds.length === 0) {
                                console.log("No companies available on initial load.");
                                companyDropdown.currentIndex = -1;
                                return;
                            }

                            let selectedId = contractModel.selectedCompanyId;
                            let selectedIndex = companyIds.indexOf(selectedId);

                            companyDropdown.currentIndex = (selectedIndex >= 0) ? selectedIndex : -1;

                            if (selectedIndex >= 0) {
                                console.log("Restored previously selected company:", selectedId);
                            } else {
                                console.log("No valid company selection found on load.");
                            }
                            // contractModel.clearSelectedContracts(); // Clear contract selection on restoration
                        }

                        // When user selects a new company, sync to backend (form resetting, backend temporary storage resetting, contract list for selected company refreshing)
                        onCurrentIndexChanged: {
                            let companyIds = contractModel.getCompanyIds(); // Ensure mapping to correct IDs
                            if (currentIndex < 0 || currentIndex >= companyIds.length) {
                                console.log("No valid company selected (index out of range).");
                                contractModel.setSelectedCompanyId(-1); // Clear selection
                                return;
                            }

                            let newSelectedId = companyIds[currentIndex];
                            contractModel.setSelectedCompanyId(newSelectedId); // Triggering selectedCompanyIdChanged()
                            console.log("User selected Company ID:", newSelectedId);
                        }

                        // Ensure contracts update automatically when company selection changes
                        Binding {
                            target: contractList
                            property: "model"
                            value: contractModel.getContractsAsVariantListByCompanyId(contractModel.selectedCompanyId)
                        }
                    }

                    // Spacer
                    Item {
                        Layout.fillWidth: true
                    }

                    // Button to open the popup
                    Button {
                        text: qsTr("Manage Contracts")
                        Layout.preferredWidth: parent.width * 0.2
                        Layout.alignment: Qt.AlignRight
                        enabled: contractModel.selectedCompanyId >= 0 // Only active when company is selected

                        onClicked: {
                            console.log("Opening Active Contracts Popup for Company ID:", contractModel.selectedCompanyId);

                            if (contractModel.selectedCompanyId < 0) {
                                globalWarningDialog.message = qsTr("Please select a company before managing contracts.");
                                globalWarningDialog.open();
                                return;
                            }

                            // Explicit refresh (just to be safe)
                            contractList.model = contractModel.getContractsAsVariantListByCompanyId(contractModel.selectedCompanyId);

                            // Check if there are any contracts to manage
                            if (contractList.model.length === 0) {
                                globalWarningDialog.message = qsTr("No active contracts available for the selected company.");
                                globalWarningDialog.open();
                                return;
                            }

                            contractModel.clearSelectedContracts(); // Clear previous selections before opening
                            activeContractsPopup.open();
                        }
                    }
                }
            }

            // Contract input form
            Frame {
                id: contractInputForm
                Layout.fillWidth: true
                padding: 0
                background: Rectangle {
                    color: "transparent"
                }

                // Form fields
                ColumnLayout {
                    id: contractForm
                    width: parent.width
                    spacing: 15

                    // Form header
                    Label {
                        text: qsTr("Contract Details")
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }

                    // Start month, start year, initial (revenue) amount
                    RowLayout {
                        spacing: 15
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50

                        Label {
                            text: qsTr("Start Month:")
                            font.pixelSize: 14
                        }

                        // Start month field
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ComboBox {
                                id: startMonthDropdown
                                model: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
                                font.pixelSize: 14

                                currentIndex: statisticsPage.formState.startMonth !== null ? statisticsPage.formState.startMonth - 1 : -1

                                onCurrentIndexChanged: {
                                    statisticsPage.formState.startMonth = currentIndex >= 0 ? currentIndex + 1 : null;

                                    statisticsPage.refreshFormHasContent();
                                    statisticsPage.refreshCanGenerateRevenue();
                                    statisticsPage.refreshCanSave();

                                    console.log(`Start Month changed: ${statisticsPage.formState.startMonth}`);
                                }
                            }

                            // Inline validation message
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: startMonthError.implicitHeight

                                Label {
                                    id: startMonthError
                                    // text: startYearField.text.length > 0 && !startYearField.acceptableInput
                                    //       ? "Please select a valid month."
                                    //       : " "
                                    color: "red"
                                    font.pixelSize: 12
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                        }

                        // Computed end month
                        // Label {
                        //     text: "End Month: " +
                        //           ((startMonthDropdown.currentIndex !== -1 && delayField.text !== "" && delayField.text !== "0")
                        //            ? (startMonthDropdown.currentIndex + 1 + parseInt(delayField.text)) % 12 || 12
                        //            : "N/A")
                        //     color: "gray"
                        //     font.pixelSize: 14
                        //     Layout.preferredWidth: 120
                        // }

                        Label {
                            text: qsTr("Start Year:")
                            font.pixelSize: 14
                        }

                        // Year field
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            TextField {
                                id: startYearField
                                placeholderText: qsTr("Enter start year")
                                font.pixelSize: 14
                                inputMethodHints: Qt.ImhDigitsOnly // Optional: show numeric keypad on touch devices
                                validator: IntValidator { bottom: 1 } // Restrict to positive integers
                                Layout.fillWidth: true

                                text: statisticsPage.formState.startYear

                                onTextChanged: {
                                    statisticsPage.formState.startYear = text;

                                    // Force immediate check
                                    startYearError.visible = text.length > 0 && !startYearField.acceptableInput;

                                    statisticsPage.refreshFormHasContent();
                                    statisticsPage.refreshCanGenerateRevenue();
                                    statisticsPage.refreshCanSave();

                                    console.log(`Start Year changed: '${statisticsPage.formState.startYear}'`);
                                }
                            }

                            // Inline validation message
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: startYearError.implicitHeight

                                Label {
                                    id: startYearError
                                    text: startYearField.text.length > 0 && !startYearField.acceptableInput
                                          ? qsTr("Please enter a valid year (e.g. 2024).")
                                          : " "  // Non-breaking space to preserve height
                                    color: "red"
                                    font.pixelSize: 12
                                    // Layout.fillHeight: true
                                    // Layout.preferredWidth: startYearField.text.length
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                        }

                        Label {
                            text: qsTr("Initial Amount (¥):")
                            font.pixelSize: 14
                        }

                        // Initial revenue amount field
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            TextField {
                                id: initialRevenueField
                                placeholderText: qsTr("Enter initial amount (¥)")
                                font.pixelSize: 14
                                inputMethodHints: Qt.ImhFormattedNumbersOnly // Suggest numeric input
                                validator: DoubleValidator { bottom: 0.0 }
                                Layout.fillWidth: true

                                text: statisticsPage.formState.initialRevenue !== null
                                      ? statisticsPage.formState.initialRevenue.toFixed(2)
                                      : ""

                                onTextChanged: {
                                    statisticsPage.formState.initialRevenue = text !== "" ? parseFloat(text) : null;

                                    // Force immediate check
                                    initialRevenueError.visible = text.length > 0 && !initialRevenueField.acceptableInput;

                                    statisticsPage.refreshFormHasContent();
                                    statisticsPage.refreshCanGenerateRevenue();
                                    statisticsPage.refreshCanSave();

                                    console.log(`Initial Revenue changed: ${statisticsPage.formState.initialRevenue}`);
                                }
                            }

                            // Inline validation message
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: initialRevenueError.implicitHeight

                                Label {
                                    id: initialRevenueError
                                    text: initialRevenueField.text.length > 0 && !initialRevenueField.acceptableInput
                                          ? qsTr("Initial revenue cannot be negative.")
                                          : " " // Non-breaking space to preserve height
                                    color: "red"
                                    font.pixelSize: 12
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                        }
                    }

                    // Form header
                    Label {
                        text: qsTr("Payment Options")
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }

                    // Multi-pay checkbox, threshold
                    RowLayout {
                        spacing: 15
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50

                        // Multi-Pay checkbox (always available)
                        CheckBox {
                            id: multiPayCheckBox
                            text: qsTr("Multi-Pay")
                            onCheckedChanged: {
                                statisticsPage.formState.multiPay = checked;

                                // Ensure UI updates dynamically
                                refreshUI();
                                statisticsPage.refreshFormHasContent();
                                statisticsPage.refreshCanGenerateRevenue();
                                statisticsPage.refreshCanSave();

                                console.log(`Multi-Pay toggled: ${checked}`);
                            }
                        }

                        // Spacer
                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Threshold (¥):")
                            font.pixelSize: 14
                        }

                        // Threshold input
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            TextField {
                                id: thresholdField
                                placeholderText: qsTr("Enter threshold (¥)")
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                validator: DoubleValidator { bottom: 0.0 }
                                font.pixelSize: 14
                                Layout.preferredWidth: parent.width * 0.4

                                text: statisticsPage.formState.threshold !== null
                                      ? statisticsPage.formState.threshold.toFixed(2)
                                      : ""

                                onTextChanged: {
                                    statisticsPage.formState.threshold = text !== "" ? parseFloat(text) : null;

                                    // Force immediate check
                                    thresholdError.visible = text.length > 0 && !thresholdField.acceptableInput;

                                    statisticsPage.refreshFormHasContent();
                                    statisticsPage.refreshCanGenerateRevenue();
                                    statisticsPage.refreshCanSave();

                                    console.log(`Threshold changed: ${statisticsPage.formState.threshold}`);
                                }
                            }

                            // Inline validation message
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: thresholdError.implicitHeight

                                Label {
                                    id: thresholdError
                                    text: thresholdField.text.length > 0 && !thresholdField.acceptableInput
                                          ? qsTr("Threshold cannot be negative.")
                                          : " "
                                    color: "red"
                                    font.pixelSize: 12
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                        }
                    }

                    // Form header
                    Label {
                        text: qsTr("Payment Scheduling")
                        font.pixelSize: 16
                        font.bold: true
                        color: Material.foreground
                    }

                    // Payment delays
                    RowLayout {
                        id: paymentDelays
                        spacing: 15
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50

                        Label {
                            text: qsTr("Payment Delay:")
                            font.pixelSize: 14
                            Layout.preferredWidth: paymentSplitLabel.width
                        }

                        // First delay input
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            TextField {
                                id: delayField
                                placeholderText: qsTr("Enter delay (in months)")
                                inputMethodHints: Qt.ImhDigitsOnly
                                validator: IntValidator { bottom: 0 }
                                font.pixelSize: 14
                                Layout.preferredWidth: 380

                                // Read from formState (UI's source of truth)
                                text: statisticsPage.formState.delay !== null
                                      ? statisticsPage.formState.delay.toString()
                                      : ""

                                // Update formState when user types
                                onTextChanged: {
                                    statisticsPage.formState.delay = text !== "" ? parseInt(text) : null;

                                    // Force immediate check
                                    delayError.visible = text.length > 0 && !delayField.acceptableInput;

                                    statisticsPage.refreshFormHasContent();
                                    statisticsPage.refreshCanGenerateRevenue();
                                    statisticsPage.refreshCanSave();

                                    console.log(`Delay changed: ${statisticsPage.formState.delay}`);
                                }
                            }

                            // Inline validation message
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: delayError.implicitHeight

                                Label {
                                    id: delayError
                                    text: delayField.text.length > 0 && !delayField.acceptableInput
                                          ? qsTr("Delay must be 0 or greater.")
                                          : " "
                                    color: "red"
                                    font.pixelSize: 12
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                        }

                        // Spacer
                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            id: secondDelayLabel
                            text: qsTr("Second\nPayment Delay:")
                            font.pixelSize: 14
                            Layout.alignment: Qt.AlignRight
                            visible: statisticsPage.formState.multiPay // Only visible when multiPay is enabled
                        }

                        // Second delay input (only available when multi-pay is enabled)
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignRight

                            TextField {
                                id: secondDelayField
                                placeholderText: qsTr("Enter second delay (in months)")
                                inputMethodHints: Qt.ImhDigitsOnly
                                validator: IntValidator { bottom: 0 }
                                font.pixelSize: 14
                                Layout.preferredWidth: delayField.width
                                // Layout.fillWidth: true
                                Layout.alignment: Qt.AlignRight
                                visible: statisticsPage.formState.multiPay // Only visible when multiPay is enabled

                                // Read from formState (UI's source of truth)
                                text: statisticsPage.formState.secondDelay !== null
                                      ? statisticsPage.formState.secondDelay.toString()
                                      : ""

                                // Update formState when user types
                                onTextChanged: {
                                    statisticsPage.formState.secondDelay = text !== "" ? parseInt(text) : null;

                                    // Force immediate check
                                    secondDelayError.visible = text.length > 0 && !secondDelayField.acceptableInput;

                                    statisticsPage.refreshFormHasContent();
                                    statisticsPage.refreshCanGenerateRevenue();
                                    statisticsPage.refreshCanSave();

                                    console.log(`Second Delay changed: ${statisticsPage.formState.secondDelay}`);
                                }
                            }

                            // Inline validation message
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: secondDelayError.implicitHeight
                                Layout.alignment: Qt.AlignRight

                                Label {
                                    id: secondDelayError
                                    text: secondDelayField.text.length > 0 && !secondDelayField.acceptableInput
                                          ? qsTr("Second delay must be 0 or greater.")
                                          : " "
                                    color: "red"
                                    font.pixelSize: 12
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                        }
                    }

                    // Payment split
                    RowLayout {
                        spacing: 15
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50

                        Label {
                            id: paymentSplitLabel
                            text: qsTr("Payment Split:")
                            font.pixelSize: 14
                        }

                        // Payment split (percentages) input
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            TextField {
                                id: percentagesField
                                placeholderText: qsTr("Enter percentages (e.g. 40/60)")
                                font.pixelSize: 14
                                inputMethodHints: Qt.ImhDigitsOnly
                                Layout.preferredWidth: delayField.width
                                // Layout.fillWidth: true

                                // Inline format validator (simple syntax check)
                                validator: RegularExpressionValidator {
                                    regularExpression: /^[0-9]+\/[0-9]+$/ // Ensure simple 2-part percentages like 40/60, 12/88, or 50/50 (basic structure)
                                }

                                text: statisticsPage.formState.percentages

                                onTextChanged: {
                                    // If multi-pay is unchecked and field is empty, auto-fill "100"
                                    const trimmed = text.trim();
                                    const multiPay = multiPayCheckBox.checked;

                                    // Auto-fill 100 when multi-pay is disabled and empty
                                    // if (!multiPay && trimmed === "") {
                                    //     text = "100";
                                    // }

                                    statisticsPage.formState.percentages = trimmed;

                                    statisticsPage.refreshFormHasContent();
                                    statisticsPage.refreshCanGenerateRevenue();
                                    statisticsPage.refreshCanSave();

                                    // Clear previous error message
                                    percentagesError.visible = false;

                                    if (multiPay) {
                                        if (trimmed.length > 0 && !percentagesField.acceptableInput) {
                                            percentagesError.text = qsTr("Enter percentages in format NN/NN (e.g. 40/60).");
                                            percentagesError.visible = true;
                                            return;
                                        }
                                    } else {
                                        if (trimmed !== "100") {
                                            percentagesError.text = qsTr("Enter 100 when multi-pay is disabled.");
                                            percentagesError.visible = true;
                                            return;
                                        }
                                    }

                                    if (!contractModel.validatePercentageString(trimmed)) {
                                        percentagesError.text = qsTr("Percentages must sum to exactly 100%.");
                                        percentagesError.visible = true;
                                    }

                                    console.log(`Percentages changed: '${statisticsPage.formState.percentages}'`);
                                }
                            }

                            // Inline validation message (inline format and semantic error from backend check)
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: percentagesError.implicitHeight

                                Label {
                                    id: percentagesError
                                    text: ""
                                    visible: false
                                    color: "red"
                                    font.pixelSize: 12
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                        }
                    }

                    // Action buttons
                    RowLayout {
                        spacing: 10
                        Layout.fillWidth: true

                        // // Formula builder button
                        // Row {
                        //     Layout.fillWidth: true
                        //     Layout.alignment: Qt.AlignLeft
                        //     spacing: 10

                        //     // Toggle icon
                        //     Image {
                        //         id: toggleIcon
                        //         source: formulaContainer.visible ? "qrc:icons/toggle-on-solid.svg" : "qrc:icons/toggle-off-solid.svg"
                        //         Layout.preferredWidth: 35
                        //         Layout.preferredHeight: 35
                        //         fillMode: Image.PreserveAspectFit
                        //         Layout.alignment: Qt.AlignLeft

                        //         MouseArea {
                        //             id: toggleMouseArea
                        //             anchors.fill: parent
                        //             cursorShape: Qt.PointingHandCursor
                        //             onClicked: {
                        //                 formulaContainer.visible = !formulaContainer.visible;
                        //             }

                        //             ToolTip {
                        //                 visible: toggleMouseArea.containsMouse
                        //                 text: formulaContainer.visible ? "Show Formula Section" : "Hide Formula Section"
                        //             }
                        //         }
                        //     }
                        // }

                        // Spacer
                        Item {
                            Layout.fillWidth: true
                        }

                        // Generate revenue, save, and reset buttons
                        Row {
                            spacing: 10
                            Layout.alignment: Qt.AlignRight

                            // Generate revenue
                            Button {
                                text: qsTr("Generate Revenue")
                                enabled: statisticsPage.canGenerateRevenue // Ensure button is only enabled when all conditions are met

                                onClicked: {
                                    console.log("User clicked Generate Revenue button.");

                                    // Extra safety check for company selection
                                    let companyId = contractModel.selectedCompanyId;
                                    if (companyId === -1) {
                                        console.warn("Generate Revenue blocked: No company selected.");
                                        globalWarningDialog.message = qsTr("Please select a company first.");
                                        globalWarningDialog.open();
                                        return;
                                    }

                                    // Collect the latest form data
                                    console.log("Collecting formState into formData map.");
                                    let formData = statisticsPage.collectFormData();

                                    // Ensure all required fields are valid
                                    if (!contractModel.canGenerateRevenue(formData)) {
                                        console.warn("Generate Revenue blocked: Form validation failed.");
                                        globalWarningDialog.message = qsTr("Please complete all required fields with valid values before generating revenue.");
                                        globalWarningDialog.open();
                                        return;
                                    }

                                    // Directly ask backend to validate, check changes, save, and generate revenue
                                    if (!contractModel.prepareAndGenerateRevenue(formData)) {
                                        console.warn("Generate Revenue failed due to validation errors: ");
                                        globalWarningDialog.message = contractModel.lastValidationError();
                                        globalWarningDialog.open();
                                        return;
                                    }

                                    // Fetch contract ID after saving
                                    let contractId = contractModel.selectMostRecentContract(companyId);
                                    if (contractId === -1) {
                                        console.error("Contract ID not found after revenue generation!");
                                        return;
                                    }

                                    // Open revenue breakdown pop-up (pop-up will auto-load data)
                                    revenuePopup.contractId = contractId;
                                    revenuePopup.companyId = companyId;
                                    revenuePopup.open();
                                }
                            }

                            // Save
                            Button {
                                text: qsTr("Save")
                                enabled: statisticsPage.canSave

                                onClicked: {
                                    console.log("User clicked Save button.");

                                    // Extra safety check for company selection (should normally never trigger if UI logic works correctly)
                                    let companyId = contractModel.selectedCompanyId;
                                    if (companyId === -1) {
                                        console.warn("Save blocked: No company selected.");
                                        globalWarningDialog.message = qsTr("Please select a company first.");
                                        globalWarningDialog.open();
                                        return;
                                    }

                                    // Collect current form data from formState (UI's version of the form)
                                    console.log("Collecting formState into formData map.");
                                    let formData = statisticsPage.collectFormData();

                                    if (!contractModel.canProceedWithSave(formData)) {
                                        // Should not happen (because button is disabled if canSave is false)
                                        console.warn("Unexpected: Save clicked despite canProceedWithSave() failing.");
                                        if (!contractModel.canSaveContract(formData)) {
                                            globalWarningDialog.message = qsTr("Please complete all required fields with valid values before saving.");
                                        } else {
                                            globalWarningDialog.message = qsTr("No changes detected, nothing to save.");
                                        }
                                        globalWarningDialog.open();
                                        return;
                                    }

                                    // Directly ask backend to validate, check changes, and save
                                    if (!contractModel.prepareAndSaveContract(formData)) {
                                        console.warn("Save blocked due to validation errors.");
                                        globalWarningDialog.message = contractModel.lastValidationError();
                                        globalWarningDialog.open();
                                        return;
                                    }

                                    console.log("Save process triggered successfully, waiting for backend confirmation...");
                                }
                            }

                            // Reset (triggering backend reset process)
                            Button {
                                text: qsTr("Reset")
                                enabled: statisticsPage.formHasContent
                                onClicked: {
                                    console.log("User clicked Reset button, initiating backend reset and clearing formState.");

                                    contractModel.resetFormForManualReset();

                                    console.log("Form reset initiated waiting for backend confirmation via formReset signal.");
                                }
                            }
                        }
                    }
                }
            }

            // // Formula builder section
            // Frame {
            //     id: formulaContainer
            //     visible: false
            //     Layout.fillWidth: true
            //     padding: 0
            //     background: Rectangle {
            //         color: "transparent"
            //     }

            //     // Formula builder rows
            //     Column {
            //         width: parent.width
            //         spacing: 10

            //         // Toolbar
            //         RowLayout {
            //             id: formulaToolbar
            //             width: parent.width
            //             spacing: 15

            //             // Predefined formulas drop-down
            //             ComboBox {
            //                 id: predefinedFormulaDropdown
            //                 model: ["Linear Revenue Growth", "Exponential Payback", "Custom Threshold Model"]
            //                 Layout.preferredWidth: parent.width * 0.27
            //                 Layout.alignment: Qt.AlignLeft
            //                 font.pixelSize: 14
            //                 ToolTip.text: "Select a predefined formula"

            //                 onCurrentIndexChanged: {
            //                     switch (currentIndex) {
            //                     case 0:
            //                         formulaInputField.text = "InitialRevenue + (Threshold * Month)";
            //                         break;
            //                     case 1:
            //                         formulaInputField.text = "InitialRevenue * (1 + Delay)^Month";
            //                         break;
            //                     case 2:
            //                         formulaInputField.text = "IF(Threshold > Delay, Threshold - Delay, 0)";
            //                         break;
            //                     }
            //                 }
            //             }

            //             // Spacer
            //             Item {
            //                 Layout.fillWidth: true
            //             }

            //             // Functions menu
            //             Row {
            //                 id: functionsRow
            //                 spacing: 10
            //                 Layout.alignment: Qt.AlignRight

            //                 Button {
            //                     text: "SUM()"
            //                     onClicked: formulaInputField.text += " SUM() "
            //                     ToolTip.text: "Summation function"
            //                 }

            //                 Button {
            //                     text: "AVERAGE()"
            //                     onClicked: formulaInputField.text += " AVERAGE() "
            //                     ToolTip.text: "Average function"
            //                 }

            //                 Button {
            //                     text: "IF()"
            //                     onClicked: formulaInputField.text += " IF(Condition, TrueValue, FalseValue) "
            //                     ToolTip.text: "Conditional function"
            //                 }

            //                 Button {
            //                     text: "MAX()"
            //                     onClicked: formulaInputField.text += " MAX(Value1, Value2) "
            //                     ToolTip.text: "Maximum of values"
            //                 }

            //                 Button {
            //                     text: "MIN()"
            //                     onClicked: formulaInputField.text += " MIN(Value1, Value2) "
            //                     ToolTip.text: "Minimum of values"
            //                 }

            //                 Button {
            //                     text: "ROUND()"
            //                     onClicked: formulaInputField.text += " ROUND(Value, DecimalPlaces) "
            //                     ToolTip.text: "Round a value"
            //                 }

            //                 Button {
            //                     text: "ABS()"
            //                     onClicked: formulaInputField.text += " ABS(Value) "
            //                     ToolTip.text: "Absolute value"
            //                 }
            //             }
            //         }

            //         // Formula input area with syntax highlighting
            //         RowLayout {
            //             width: parent.width
            //             spacing: 15

            //             // Formula input area and feedback rows
            //             Column {
            //                 spacing: 5
            //                 Layout.fillWidth: true

            //                 // Formula input area (text box)
            //                 TextArea {
            //                     id: formulaInputField
            //                     placeholderText: "Type or build your formula here..."
            //                     width: parent.width
            //                     height: 70
            //                     font.pixelSize: 14
            //                     wrapMode: TextEdit.Wrap

            //                     onTextChanged: {
            //                         const isValid = validateFormula(text);
            //                         formulaValidationText.text = isValid ? "Valid formula" : "Invalid formula";
            //                         formulaValidationText.color = isValid ? "green" : "red";
            //                     }
            //                 }

            //                 // Validation feedback
            //                 Text {
            //                     id: formulaValidationText
            //                     text: "Invalid formula"
            //                     color: "red"
            //                     font.pixelSize: 14
            //                 }
            //             }
            //         }
            //     }
            // }
        }
    }

    // Active contracts table pop-up
    Popup {
        id: activeContractsPopup
        modal: true
        width: parent.width * 0.8
        height: parent.height * 0.8
        anchors.centerIn: parent

        // Selection properties
        property int selectedContractId: -1
        property int lastLoadedContractId: -1

        // Ensure proper deselection when closing popup
        onClosed: {
            console.log("Popup closed. Clearing selections.");
            activeContractsPopup.selectedContractId = -1;
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20

            // Header
            Text {
                text: qsTr("Active Contracts")
                font.pixelSize: 20
                font.bold: true
                color: Material.foreground
            }

            RowLayout {
                spacing: 20
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                Text { text: ""; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignHCenter; } // Checkbox spacer
                Text { text: qsTr("Start Month"); Layout.preferredWidth: 80; horizontalAlignment: Text.AlignHCenter; font.bold: true }
                Text { text: qsTr("Start Year"); Layout.preferredWidth: 80; horizontalAlignment: Text.AlignHCenter; font.bold: true }
                Text { text: qsTr("Revenue"); Layout.preferredWidth: 100; horizontalAlignment: Text.AlignHCenter; font.bold: true }
                Text { text: qsTr("Delay"); Layout.preferredWidth: 80; horizontalAlignment: Text.AlignHCenter;font.bold: true }
                Text { text: qsTr("Second Delay"); Layout.preferredWidth: 80; horizontalAlignment: Text.AlignHCenter; font.bold: true }
                Text { text: qsTr("Threshold"); Layout.preferredWidth: 100; horizontalAlignment: Text.AlignHCenter; font.bold: true }
                Text { text: qsTr("Percentages"); Layout.preferredWidth: 120; horizontalAlignment: Text.AlignHCenter; font.bold: true }
            }

            // Contract list
            ListView {
                id: contractList
                property var popupRef: activeContractsPopup
                Layout.preferredWidth: parent.width
                Layout.preferredHeight: parent.height * 0.6
                model: contractModel.getContractsAsVariantListByCompanyId(contractModel.selectedCompanyId)
                clip: true

                delegate: Rectangle {
                    width: parent.width
                    height: 40
                    // color: contractModel.getSelectedContracts().includes(modelData.contractId) ? "#c3b3d0"
                    //                                                                            : activeContractsPopup.lastLoadedContractId === modelData.contractId ? "#a38bbf"
                    //                                                                                                                                                 : Material.background
                    color: activeContractsPopup.selectedContractId === modelData.contractId
                           ? "#c3b3d0"  // Highlight selected contract
                           : activeContractsPopup.lastLoadedContractId === modelData.contractId
                             ? "#a38bbf" // Subtle highlight for last loaded contract
                             : Material.background
                    radius: Material.LargeScale // Rounded corners
                    border.color: Material.accent // Accent color for the border
                    border.width: 1

                    // Contract row (selectable) and delete button
                    RowLayout {
                        anchors.fill: parent
                        spacing: 10

                        // Contract row (selectable part)
                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40

                            RowLayout {
                                anchors.fill: parent
                                spacing: 10

                                // Checkbox for single-selection
                                CheckBox {
                                    checked: activeContractsPopup.selectedContractId === modelData.contractId
                                    // checked: contractModel.isContractSelected(modelData.contractId)

                                    onClicked: {
                                        // If already selected, unselect it; otherwise, select it
                                        if (activeContractsPopup.selectedContractId === modelData.contractId) {
                                            activeContractsPopup.selectedContractId = -1;
                                        } else {
                                            activeContractsPopup.selectedContractId = modelData.contractId;
                                        }
                                        console.log("Selected contract via checkbox:", activeContractsPopup.selectedContractId);
                                    }
                                    // onClicked: {
                                    //     contractModel.toggleContractSelection(modelData.contractId);
                                    // }
                                }

                                // Contract details
                                Text { text: modelData.startMonth; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignHCenter }
                                Text { text: modelData.startYear; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignHCenter }
                                Text { text: modelData.initialRevenue; Layout.preferredWidth: 100; horizontalAlignment: Text.AlignHCenter }
                                Text { text: modelData.delay; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignHCenter }

                                Text {
                                    text: modelData.multiPay ? modelData.secondDelay : "-" // Show only if `multiPay` is true
                                    Layout.preferredWidth: 80
                                    horizontalAlignment: Text.AlignHCenter
                                    // visible: modelData.multiPay // Hide if `multiPay` is false
                                }

                                Text { text: modelData.threshold; Layout.preferredWidth: 100; horizontalAlignment: Text.AlignHCenter }
                                Text { text: modelData.percentages; Layout.preferredWidth: 120; horizontalAlignment: Text.AlignHCenter }
                            }

                            // Row selection (single contract selection), synchronized with checkbox
                            MouseArea {
                                anchors.fill: parent

                                onClicked: {
                                    if (activeContractsPopup.selectedContractId === modelData.contractId) {
                                        activeContractsPopup.selectedContractId = -1;
                                    } else {
                                        activeContractsPopup.selectedContractId = modelData.contractId;
                                    }
                                    console.log("Selected contract via row click:", activeContractsPopup.selectedContractId);
                                }

                                // contractList.forceLayout(); // Ensure immediate visual update
                                // onClicked: {
                                //     contractModel.toggleContractSelection(modelData.contractId);
                                // }
                                // preventStealing: true
                            }
                        }

                        // Delete
                        Button {
                            // Layout.preferredWidth: 40
                            // Layout.preferredHeight: 40
                            background: Rectangle {
                                color: "transparent"
                            }

                            // Trash bin icon
                            Image {
                                id: trashBinIcon
                                source: "qrc:/icons/trash-solid.svg"
                                width: 20
                                height: 20
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                anchors.centerIn: parent
                            }

                            onClicked: {
                                // contractModel.removeContract(contractModel.selectedCompanyId, modelData.contractId);
                                // let index = activeContractsPopup.selectedContractIds.indexOf(modelData.contractId);
                                // if (index !== -1) activeContractsPopup.selectedContractIds.splice(index, 1);

                                // console.log("Deleted contract:", modelData.contractId);

                                console.log("Attempting to delete contract:", modelData.contractId);

                                // Remove contract from backend
                                contractModel.removeContract(contractModel.selectedCompanyId, modelData.contractId);

                                var popup = parent
                                while (popup && !popup.popupRef) {
                                    popup = popup.parent
                                }

                                if (popup) {
                                    if (popup.popupRef.selectedContractId === modelData.contractId) {
                                        popup.popupRef.selectedContractId = -1;
                                        console.log("Cleared selected contract after deletion.");
                                    }
                                    if (popup.popupRef.lastLoadedContractId === modelData.contractId) {
                                        popup.popupRef.lastLoadedContractId = -1;
                                        console.log("Cleared last loaded contract after deletion.");
                                        contractModel.resetFormForManualReset();
                                    }
                                }

                                // console.log("Deleted contract:", modelData.contractId);
                            }
                        }
                    }
                }
            }

            // Backend event handlers
            Connections {
                target: contractModel

                // Triggered when contract selection changes
                function onSelectedContractChanged() {
                    contractList.model = contractModel.getContractsAsVariantListByCompanyId(contractModel.selectedCompanyId);
                }

                // Triggered when a single contract is added, edited, or removed
                function onContractChanged(companyId) {
                    if (companyId === contractModel.selectedCompanyId) {
                        contractList.model = contractModel.getContractsAsVariantListByCompanyId(companyId);
                        console.log("Contract list updated due to a contract change for Company ID:", companyId);

                        // Ensure only one contract is selected at a time
                        if (activeContractsPopup.selectedContractId !== -1) {
                            let stillExists = contractList.model.some(contract => contract.contractId === activeContractsPopup.selectedContractId);
                            if (!stillExists) {
                                console.log("Selected contract no longer exists. Clearing selection.");
                                activeContractsPopup.selectedContractId = -1;
                            }
                        }

                        // If a contract was just saved, reset highlight
                        if (activeContractsPopup.lastLoadedContractId !== -1) {
                            activeContractsPopup.lastLoadedContractId = -1;
                            activeContractsPopup.selectedContractId = -1;
                            console.log("Clearing selection after contract save.");
                        }
                    }
                }

                // Triggered when all contracts are reloaded (bulk update)
                function onContractsReloaded() {
                    if (contractModel.selectedCompanyId === -1) {
                        console.warn("No company selected. Skipping contract refresh.");
                        return;
                    }

                    // Refresh the active contracts table
                    contractList.model = contractModel.getContractsAsVariantListByCompanyId(contractModel.selectedCompanyId);
                    console.log("Contract list refreshed after bulk update.");

                    // If selected contract no longer exists, clear selection
                    let stillExists = contractList.model.some(contract => contract.contractId === activeContractsPopup.selectedContractId);
                    if (!stillExists) {
                        console.log("Selected contract no longer exists. Resetting selectedContractId.");
                        activeContractsPopup.selectedContractId = -1;
                    }

                    // If last loaded contract no longer exists, reset it too
                    let lastExists = contractList.model.some(contract => contract.contractId === activeContractsPopup.lastLoadedContractId);
                    if (!lastExists) {
                        console.log("Last loaded contract no longer exists. Resetting lastLoadedContractId.");
                        activeContractsPopup.lastLoadedContractId = -1;
                    }
                }
            }

            // Action buttons
            Row {
                spacing: 10
                Layout.alignment: Qt.AlignRight

                // Load contract
                Button {
                    text: qsTr("Load Contract")
                    enabled: activeContractsPopup.selectedContractId !== -1

                    onClicked: {
                        let companyId = contractModel.selectedCompanyId;
                        let contractId = activeContractsPopup.selectedContractId;

                        if (contractId === -1) {
                            console.warn("No contract selected. Cannot load.");
                            globalWarningDialog.message = qsTr("Please select a contract to load.");
                            globalWarningDialog.open();
                            return;
                        }

                        console.log("Loading contract for Company ID:", companyId, "Contract ID:", contractId);

                        console.log("Requesting backend to load contract:", contractId, "for company:", companyId);
                        contractModel.loadContractIntoTemp(companyId, contractId);

                        activeContractsPopup.lastLoadedContractId = contractId;
                        activeContractsPopup.selectedContractId = -1;
                        activeContractsPopup.close();
                    }
                }

                // Cancel
                Button {
                    text: qsTr("Cancel")

                    onClicked: {
                        console.log("Cancelling contract selection...");

                        activeContractsPopup.selectedContractId = -1; // Reset only temporary selection
                        activeContractsPopup.close();
                    }
                }
            }
        }
    }

    // Revenue breakdown pop-up
    Popup {
        id: revenuePopup
        modal: true
        width: parent.width * 0.8
        height: parent.height * 0.8
        anchors.centerIn: parent

        property int contractId: -1 // Contract for which revenue is shown
        property int companyId: -1

        ListModel {
            id: revenueModel
        }

        // Automatically load revenue data when the pop-up opens
        onOpened: {
            console.log("Revenue pop-up opened. Loading revenue data...");
            loadRevenueData(companyId, contractId);
        }

        // Load revenue data
        function loadRevenueData(companyId, contractId) {
            console.log("Fetching revenue for contract:", contractId);

            let revenueList = contractModel.loadRevenueForContract(companyId, contractId);
            console.log("Received revenue data:", JSON.stringify(revenueList));

            // Ensure list is valid before populating model
            if (!revenueList || revenueList.length === 0) {
                console.warn("No revenue data found.");
                globalWarningDialog.message = "No revenue data available.";
                globalWarningDialog.open();
                return;
            }

            // Clear existing data
            revenueModel.clear();

            // Validate and populate revenue list
            for (let i = 0; i < revenueList.length; i++) {
                let revItem = revenueList[i];

                // Print each item before adding
                console.log(`Processing revenue entry ${i}:`, JSON.stringify(revItem));

                // Validate data before appending
                if (revItem && revItem.year !== undefined && revItem.month !== undefined && revItem.revenue !== undefined) {
                    revenueModel.append({
                                            year: revItem.year,
                                            month: revItem.month,
                                            revenue: revItem.revenue,
                                            originalRevenue: revItem.originalRevenue || revItem.revenue,
                                            percentageApplied: revItem.percentageApplied || 100
                                        });
                } else {
                    console.error("Invalid revenue data entry at index", i, ":", JSON.stringify(revItem));
                }
            }

            console.log("Revenue breakdown loaded successfully.");
            console.log("Final model contents:", JSON.stringify(revenueModel.get(0))); // Debugging output
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 20
            anchors.margins: 20

            // Header
            Text {
                text: qsTr("Revenue Breakdown")
                color: Material.foreground
                font.pixelSize: 20
                font.bold: true
            }

            // Revenue table container
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    // Table header
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40

                        RowLayout {
                            anchors.fill: parent
                            spacing: 10

                            Rectangle {
                                Layout.fillWidth: true
                                // Layout.preferredWidth: parent.width * 0.1
                                Layout.preferredHeight: 40
                                radius: Material.LargeScale // Rounded corners
                                color: "#f0f0f0"
                                border.color: Material.accent // Accent color for the border
                                border.width: 1

                                Text {
                                    text: qsTr("Year")
                                    font.bold: true
                                    font.pixelSize: 13
                                    anchors.centerIn: parent
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                // Layout.preferredWidth: parent.width * 0.1
                                Layout.preferredHeight: 40
                                radius: Material.LargeScale // Rounded corners
                                color: "#f0f0f0"
                                border.color: Material.accent // Accent color for the border
                                border.width: 1

                                Text {
                                    text: qsTr("Month")
                                    font.bold: true
                                    font.pixelSize: 13
                                    anchors.centerIn: parent
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                // Layout.preferredWidth: parent.width * 0.1
                                Layout.preferredHeight: 40
                                radius: Material.LargeScale // Rounded corners
                                border.color: Material.accent // Accent color for the border
                                border.width: 1

                                Text {
                                    text: qsTr("Original Revenue (¥)")
                                    font.bold: true
                                    font.pixelSize: 13
                                    anchors.centerIn: parent
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                // Layout.preferredWidth: parent.width * 0.1
                                Layout.preferredHeight: 40
                                radius: Material.LargeScale // Rounded corners
                                border.color: Material.accent // Accent color for the border
                                border.width: 1

                                Text {
                                    text: qsTr("Applied %")
                                    font.bold: true
                                    font.pixelSize: 13
                                    anchors.centerIn: parent
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                // Layout.preferredWidth: parent.width * 0.1
                                Layout.preferredHeight: 40
                                radius: Material.LargeScale // Rounded corners
                                color: "#f0f0f0"
                                border.color: Material.accent // Accent color for the border
                                border.width: 1

                                Text {
                                    text: qsTr("Final Revenue (¥)")
                                    font.bold: true
                                    font.pixelSize: 13
                                    anchors.centerIn: parent
                                }
                            }

                            // Rectangle {
                            //     Layout.preferredWidth: parent.width * 0.3
                            //     Layout.preferredHeight: 40
                            //     radius: Material.LargeScale // Rounded corners
                            //     border.color: Material.accent // Accent color for the border
                            //     border.width: 1

                            //     Text {
                            //         text: "Revenue Amount (¥)"
                            //         font.bold: true
                            //         font.pixelSize: 13
                            //         anchors.centerIn: parent
                            //     }
                            // }
                        }
                    }

                    // Table content
                    ListView {
                        id: revenueListView
                        // Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: parent.width
                        // Layout.preferredHeight: parent.height * 0.6
                        clip: true
                        model: revenueModel

                        delegate: Item {
                            width: ListView.view.width
                            height: 40

                            required property int year
                            required property int month
                            required property real originalRevenue
                            required property string percentageApplied
                            required property real revenue

                            RowLayout {
                                spacing: 10
                                anchors.fill: parent

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 40

                                    Text {
                                        text: year
                                        anchors.centerIn: parent
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 40

                                    Text {
                                        text: month
                                        anchors.centerIn: parent
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 40

                                    Text {
                                        text: originalRevenue.toFixed(2)
                                        anchors.centerIn: parent
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 40

                                    Text {
                                        text: percentageApplied
                                        anchors.centerIn: parent
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 40

                                    Text {
                                        text: revenue.toFixed(2)
                                        anchors.centerIn: parent
                                    }
                                }

                                // Rectangle {
                                //     width: parent.width * 0.4
                                //     height: 40

                                //     Text {
                                //         text: revenue.toFixed(2)
                                //         anchors.centerIn: parent
                                //     }
                                // }
                            }
                        }
                    }
                }
            }

            // Close
            Button {
                text: qsTr("Close")
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    revenuePopup.close();
                }
            }
        }
    }
}
