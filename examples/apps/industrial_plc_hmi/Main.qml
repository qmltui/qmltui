import Qtui.Extras
import Qtui.Canvas
import QtQml 2.15
import Qtui
import Qtui.Controls
import IndustrialPlcHmi

Terminal {
    id: rootWindow
    fillTerminal: true
    color: "black"
    useAlternateScreen: true

    // Active View Tab State
    property int activeTab: 0 // 0: Dashboard, 1: Alarms, 2: Diagnostics, 3: Settings

    // PLC State Variables
    property string machineState: "idle" // idle, processing, error, maintenance
    property int speed: 0
    property int targetSpeed: 1500
    property double temperature: 28.5
    property double maxTempLimit: 85.0
    property double pressure: 1.0
    property int cycles: 0
    property int errorCount: 0
    property bool valveA: false
    property bool valveB: false
    property bool alarmBlinking: false
    property string pendingAction: ""
    
    // Log/Status History (shows latest events)
    property var logMessages: ["SYSTEM INITIALIZED", "STATE CHANGED TO: IDLE", "READY FOR OPERATOR COMMANDS"]
    
    // Temperature & Telemetry History (rolling data points for plotting and sparklines)
    property var tempHistory: [28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5]
    property var speedHistory: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    property var pressureHistory: [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]

    // Mandatory Quit key handler as per workspace rule + Navigation & Control Shortcuts
    Keys.onPressed: (event) => {
        if (confirmDlg.visible) {
            if (event.key === Qt.Key_Y) {
                executeConfirmedAction();
                confirmDlg.visible = false;
                event.accepted = true;
            } else if (event.key === Qt.Key_N || event.key === Qt.Key_Escape) {
                pendingAction = "";
                confirmDlg.visible = false;
                event.accepted = true;
            }
            return; // Modal active, ignore other keys
        }

        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return;
        }

        // View Tab Switching Shortcuts
        if (event.key === Qt.Key_F1 || event.key === Qt.Key_1) {
            activeTab = 0;
            event.accepted = true;
            return;
        } else if (event.key === Qt.Key_F2 || event.key === Qt.Key_2) {
            activeTab = 1;
            event.accepted = true;
            return;
        } else if (event.key === Qt.Key_F3 || event.key === Qt.Key_3) {
            activeTab = 2;
            event.accepted = true;
            return;
        } else if (event.key === Qt.Key_F4 || event.key === Qt.Key_4) {
            activeTab = 3;
            event.accepted = true;
            return;
        }

        // Functional Shortcut Keys (active when input fields are not focused)
        let inputsActive = rpmInput.activeFocus || tempLimitInput.activeFocus;
        if (!inputsActive) {
            if (event.key === Qt.Key_S) {
                triggerStart();
                event.accepted = true;
            } else if (event.key === Qt.Key_X) {
                triggerStop();
                event.accepted = true;
            } else if (event.key === Qt.Key_R) {
                requestReset();
                event.accepted = true;
            } else if (event.key === Qt.Key_M) {
                requestMaintenance();
                event.accepted = true;
            }
        }
    }

    // Helper functions
    function log(msg) {
        let temp = logMessages;
        temp.unshift(msg);
        if (temp.length > 10) {
            temp.pop();
        }
        logMessages = temp;
    }

    function pushHistory(newTemp, newSpeed, newPressure) {
        var tArr = [];
        for (var i = 1; i < tempHistory.length; i++) tArr.push(tempHistory[i]);
        tArr.push(newTemp);
        tempHistory = tArr;

        var sArr = [];
        for (var j = 1; j < speedHistory.length; j++) sArr.push(speedHistory[j]);
        sArr.push(newSpeed);
        speedHistory = sArr;

        var pArr = [];
        for (var k = 1; k < pressureHistory.length; k++) pArr.push(pressureHistory[k]);
        pArr.push(newPressure);
        pressureHistory = pArr;
    }

    function getPlotPoints(canvasWidth, canvasHeight) {
        if (canvasWidth <= 0 || canvasHeight <= 0) return [];
        let w = canvasWidth * 2;
        let h = canvasHeight * 4;
        let pts = [];
        let len = tempHistory.length;
        let minT = 20.0;
        let maxT = 100.0;
        let range = maxT - minT;

        for (let i = 0; i < len; i++) {
            let cx = Math.round((i / (len - 1)) * (w - 1));
            let cy = Math.round((h - 1) - ((tempHistory[i] - minT) / range) * (h - 1));
            cy = Math.max(0, Math.min(h - 1, cy));
            pts.push({ x: cx, y: cy });
        }
        return pts;
    }

    function triggerStart() {
        if (machineState !== "error") {
            machineState = "processing";
            log("STATE: PROCESSING (START CMD)");
        }
    }

    function triggerStop() {
        if (machineState !== "error") {
            machineState = "idle";
            log("STATE: IDLE (STOP CMD)");
        }
    }

    function requestReset() {
        if (machineState === "error") {
            pendingAction = "reset";
            confirmDlg.title = "CLEAR FAULT ALARM";
            confirmDlg.message = "Acknowledge and reset over-temperature alarm lockout?";
            confirmDlg.visible = true;
            confirmDlg.focus = true;
        }
    }

    function requestMaintenance() {
        if (machineState !== "error") {
            pendingAction = "maint";
            confirmDlg.title = "MAINTENANCE MODE";
            confirmDlg.message = machineState === "maintenance" 
                ? "Exit maintenance mode and return station to IDLE?"
                : "Switch PLC station into low-speed MAINTENANCE mode?";
            confirmDlg.visible = true;
            confirmDlg.focus = true;
        }
    }

    function executeConfirmedAction() {
        if (pendingAction === "reset") {
            machineState = "idle";
            temperature = 35.0; // Cool down
            log("STATE: IDLE (ALARM RESET)");
        } else if (pendingAction === "maint") {
            if (machineState === "maintenance") {
                machineState = "idle";
                log("STATE: IDLE (MAINT RUN COMPLETED)");
            } else {
                machineState = "maintenance";
                log("STATE: MAINTENANCE ACTIVE");
            }
        }
        pendingAction = "";
    }

    // Telemetry & States Simulation Timer
    Timer {
        id: simTimer
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            if (rootWindow.machineState === "processing") {
                rootWindow.cycles += 1;
                
                // Ramp speed up towards setpoint
                if (rootWindow.speed < rootWindow.targetSpeed) {
                    rootWindow.speed = Math.min(rootWindow.targetSpeed, rootWindow.speed + 200);
                } else if (rootWindow.speed > rootWindow.targetSpeed) {
                    rootWindow.speed = Math.max(rootWindow.targetSpeed, rootWindow.speed - 200);
                }
                
                // Temp goes up based on speed
                let tempFactor = (rootWindow.speed / 1000.0) * 1.8;
                rootWindow.temperature = Math.min(110.0, rootWindow.temperature + tempFactor + (Math.random() * 1.5 - 0.5));
                
                // Pressure fluctuates relative to temp
                rootWindow.pressure = 1.0 + (rootWindow.temperature - 25.0) * 0.06 + (Math.random() * 0.1 - 0.05);
                
                // Valve simulation sequence
                rootWindow.valveA = (rootWindow.cycles % 8 < 4);
                rootWindow.valveB = (rootWindow.cycles % 8 >= 4);

                // Check safety limit
                if (rootWindow.temperature >= rootWindow.maxTempLimit) {
                    rootWindow.machineState = "error";
                    rootWindow.errorCount += 1;
                    rootWindow.log("ALARM: OVER-TEMPERATURE FAULT!");
                }
            } else if (rootWindow.machineState === "idle" || rootWindow.machineState === "error") {
                // Decay/Cool down
                if (rootWindow.speed > 0) {
                    rootWindow.speed = Math.max(0, rootWindow.speed - 350);
                }
                if (rootWindow.temperature > 28.5) {
                    rootWindow.temperature = Math.max(28.5, rootWindow.temperature - 2.5);
                }
                if (rootWindow.pressure > 1.0) {
                    rootWindow.pressure = Math.max(1.0, rootWindow.pressure - 0.3);
                }
                rootWindow.valveA = false;
                rootWindow.valveB = false;
            } else if (rootWindow.machineState === "maintenance") {
                // Maintenance low speed check
                rootWindow.speed = 150;
                rootWindow.temperature = 31.0;
                rootWindow.pressure = 1.1;
                rootWindow.valveA = true;
                rootWindow.valveB = false;
            }

            // Periodic operational logging
            if (rootWindow.machineState === "processing" && rootWindow.cycles % 5 === 0) {
                rootWindow.log("PROCESS CYCLE " + rootWindow.cycles + " | T: " + rootWindow.temperature.toFixed(1) + "C");
            }

            // Push to telemetry history
            rootWindow.pushHistory(rootWindow.temperature, rootWindow.speed, rootWindow.pressure);

            // Update Braille Plot points
            if (tempCanvas.width > 0 && tempCanvas.height > 0) {
                tempCanvas.updatePlot();
            }
        }
    }

    // Alarm screen blinking timer
    Timer {
        id: blinkTimer
        interval: 500
        running: rootWindow.machineState === "error"
        repeat: true
        onTriggered: {
            rootWindow.alarmBlinking = !rootWindow.alarmBlinking;
        }
    }

    // Main layout wrapper
    Rectangle {
        id: mainRect
        anchors.fill: parent
        color: rootWindow.machineState === "error" 
               ? (rootWindow.alarmBlinking ? HmiTheme.bgError : HmiTheme.bgErrorAlt) 
               : (rootWindow.machineState === "maintenance" ? HmiTheme.bgMaintenance : (rootWindow.machineState === "processing" ? HmiTheme.bgProcessing : HmiTheme.bgIdle))

        // 1. HEADER PANEL
        HeaderPanel {
            id: headerPanel
            machineState: rootWindow.machineState
            stationId: "STN#42"
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
        }

        // 2. NAVIGATION TAB BAR
        NavTabBar {
            id: navBar
            currentTab: rootWindow.activeTab
            alarmCount: rootWindow.errorCount
            anchors.top: headerPanel.bottom
            anchors.left: parent.left
            anchors.right: parent.right
        }

        // 3. MAIN CONTENT VIEW STACK
        Rectangle {
            id: viewContainer
            anchors.top: navBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: footerPanel.top
            color: "transparent"

            // VIEW 0: MAIN DASHBOARD VIEW
            Rectangle {
                anchors.fill: parent
                visible: rootWindow.activeTab === 0
                color: "transparent"

                // SIDEBAR TELEMETRY PANEL (LEFT)
                Rectangle {
                    id: sidebarPanel
                    width: Math.max(24, Math.floor(parent.width * 0.28))
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    border.style: "single"
                    border.color: HmiTheme.borderNormal

                    Column {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 1

                        Text {
                            text: "📊 TELEMETRY & GAUGES"
                            color: HmiTheme.textSecondary
                            bold: true
                        }

                        Gauge {
                            label: "Speed"
                            value: rootWindow.speed
                            unit: "RPM"
                            minVal: 0
                            maxVal: 3000
                            warningVal: 2500
                            dangerVal: 2800
                            decimals: 0
                        }
                        Sparkline {
                            width: parent.width
                            height: 1
                            values: rootWindow.speedHistory
                            minVal: 0
                            maxVal: 3000
                            color: HmiTheme.statusProcessing
                        }

                        Gauge {
                            label: "Temp"
                            value: rootWindow.temperature
                            unit: "°C"
                            minVal: 20
                            maxVal: 100
                            warningVal: 75
                            dangerVal: rootWindow.maxTempLimit
                            decimals: 1
                        }
                        Sparkline {
                            width: parent.width
                            height: 1
                            values: rootWindow.tempHistory
                            minVal: 20
                            maxVal: 100
                            color: rootWindow.temperature >= rootWindow.maxTempLimit ? HmiTheme.statusError : HmiTheme.textWarning
                        }

                        Gauge {
                            label: "Pres"
                            value: rootWindow.pressure
                            unit: "bar"
                            minVal: 0
                            maxVal: 5
                            warningVal: 3.5
                            dangerVal: 4.5
                            decimals: 2
                        }
                        Sparkline {
                            width: parent.width
                            height: 1
                            values: rootWindow.pressureHistory
                            minVal: 0
                            maxVal: 5
                            color: HmiTheme.textHighlight
                        }

                        Text {
                            text: "⚙ ACTUATORS & FLOW"
                            color: HmiTheme.textSecondary
                            bold: true
                        }

                        Row {
                            width: parent.width
                            spacing: 1
                            ActuatorLed { name: "V-A"; active: rootWindow.valveA }
                            ActuatorLed { name: "V-B"; active: rootWindow.valveB }
                        }

                        // Animated conveyor flow pulse
                        Rectangle {
                            id: conveyorPulseRect
                            width: parent.width
                            height: 1
                            color: "transparent"

                            property real pulsePos: 0
                            NumberAnimation {
                                target: conveyorPulseRect
                                property: "pulsePos"
                                running: rootWindow.machineState === "processing" || rootWindow.machineState === "maintenance"
                                from: 0
                                to: 10
                                duration: rootWindow.machineState === "maintenance" ? 2000 : 800
                                loops: -1
                            }

                            Text {
                                anchors.centerIn: parent
                                text: {
                                    if (rootWindow.machineState === "processing") {
                                        let chars = [">  ", ">> ", ">>>", " >>", "  >", "   ", ">  ", ">> ", ">>>", " >>"];
                                        return "[ FLOW: " + chars[parent.pulsePos % chars.length] + " ]";
                                    } else if (rootWindow.machineState === "maintenance") {
                                        return "[ MAINT: " + (parent.pulsePos % 2 === 0 ? "◧" : "◨") + " ]";
                                    } else if (rootWindow.machineState === "error") {
                                        return "[ 🛑 LOCKED ]";
                                    } else {
                                        return "[ ■ IDLE ]";
                                    }
                                }
                                color: rootWindow.machineState === "processing" ? HmiTheme.textSuccess : (rootWindow.machineState === "error" ? HmiTheme.textDanger : HmiTheme.textDim)
                                bold: true
                            }
                        }

                        Text {
                            text: "📈 METRICS"
                            color: HmiTheme.textSecondary
                            bold: true
                        }

                        Text { text: "Cycles: " + rootWindow.cycles; color: HmiTheme.textPrimary }
                        Text { text: "Alarms: " + rootWindow.errorCount; color: rootWindow.errorCount > 0 ? HmiTheme.textDanger : HmiTheme.textPrimary; bold: rootWindow.errorCount > 0 }
                    }
                }

                // TELEMETRY GRAPH PANEL (CENTER)
                Rectangle {
                    id: graphPanel
                    anchors.top: parent.top
                    anchors.left: sidebarPanel.right
                    anchors.right: inputPanel.left
                    anchors.bottom: parent.bottom
                    border.style: "single"
                    border.color: HmiTheme.borderNormal

                    Column {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 1

                        Text {
                            text: "📈 TEMP TREND (20-100°C)"
                            color: HmiTheme.textSecondary
                            bold: true
                        }

                        Text {
                            text: "CUR: " + rootWindow.temperature.toFixed(1) + "°C | MAX: " + rootWindow.maxTempLimit.toFixed(1) + "°C"
                            color: rootWindow.temperature >= rootWindow.maxTempLimit ? HmiTheme.textDanger : HmiTheme.textHighlight
                        }

                        Rectangle {
                            width: parent.width
                            height: Math.max(5, parent.height - 4)
                            color: "transparent"

                            Row {
                                anchors.fill: parent
                                spacing: 1

                                Column {
                                    width: 5
                                    height: parent.height

                                    Text { text: "100°"; color: HmiTheme.textDim }

                                    Item {
                                        width: 5
                                        height: Math.max(0, parent.height - 2)
                                        Text {
                                            anchors.centerIn: parent
                                            text: " 60°"
                                            color: HmiTheme.textDim
                                        }
                                    }

                                    Text { text: " 20°"; color: HmiTheme.textDim }
                                }

                                Rectangle {
                                    width: parent.width - 6
                                    height: parent.height
                                    border.style: "ascii"
                                    border.color: "#444444"

                                    Canvas {
                                        id: tempCanvas
                                        anchors.fill: parent
                                        anchors.margins: 1
                                        color: rootWindow.machineState === "error" ? HmiTheme.statusError : (rootWindow.machineState === "processing" ? HmiTheme.textHighlight : HmiTheme.statusIdle)

                                        onWidthChanged: updatePlot()
                                        onHeightChanged: updatePlot()

                                        function updatePlot() {
                                            if (width > 0 && height > 0) {
                                                tempPoly.points = rootWindow.getPlotPoints(width, height);
                                                let h4 = height * 4 - 1;
                                                let ly = Math.max(0, Math.min(Math.floor(h4), Math.floor(h4 - ((rootWindow.maxTempLimit - 20.0) / 80.0) * h4)));
                                                limitLine.y1 = ly;
                                                limitLine.y2 = ly;
                                                limitLine.x2 = Math.floor(width * 2);
                                            }
                                        }

                                        Line {
                                            id: limitLine
                                            x1: 0
                                            y1: 0
                                            x2: 100
                                            y2: 0
                                        }

                                        Polygon {
                                            id: tempPoly
                                            closed: false
                                        }
                                    }
                                }
                            }
                        }

                        Text {
                            text: "<- 30s history           NOW ->"
                            color: HmiTheme.textDim
                        }
                    }
                }

                // CONTROLS PANEL (RIGHT)
                Rectangle {
                    id: inputPanel
                    width: Math.max(24, Math.floor(parent.width * 0.26))
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    border.style: "single"
                    border.color: HmiTheme.borderNormal

                    Column {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 1

                        Text {
                            text: "🛠️ CONTROLS"
                            color: HmiTheme.textSecondary
                            bold: true
                        }

                        Text { text: "RPM Setpoint:"; color: HmiTheme.textPrimary }
                        Rectangle {
                            width: parent.width
                            height: 3
                            border.style: rpmInput.activeFocus ? "double" : "single"
                            border.color: rpmInput.activeFocus ? HmiTheme.borderFocus : HmiTheme.borderNormal

                            TextInput {
                                id: rpmInput
                                anchors.fill: parent
                                anchors.margins: 1
                                text: rootWindow.targetSpeed.toString()
                                color: HmiTheme.textPrimary
                                onAccepted: {
                                    rootWindow.targetSpeed = parseInt(rpmInput.text);
                                    rootWindow.log("SET RPM -> " + rpmInput.text);
                                }
                            }
                        }

                        Text { text: "Temp Limit:"; color: HmiTheme.textPrimary }
                        Rectangle {
                            width: parent.width
                            height: 3
                            border.style: tempLimitInput.activeFocus ? "double" : "single"
                            border.color: tempLimitInput.activeFocus ? HmiTheme.borderFocus : HmiTheme.borderNormal

                            TextInput {
                                id: tempLimitInput
                                anchors.fill: parent
                                anchors.margins: 1
                                text: rootWindow.maxTempLimit.toString()
                                color: HmiTheme.textPrimary
                                onAccepted: {
                                    rootWindow.maxTempLimit = parseFloat(tempLimitInput.text);
                                    rootWindow.log("LIMIT TEMP -> " + tempLimitInput.text + "C");
                                }
                            }
                        }

                        Row {
                            width: parent.width
                            height: 3
                            spacing: 1
                            Button {
                                width: Math.floor((parent.width - 1) / 2)
                                height: 3
                                label: "[S] START"
                                normalColor: "#1e3a1e"
                                focusColor: "#2e5a2e"
                                textColor: HmiTheme.textSuccess
                                onClicked: triggerStart()
                            }
                            Button {
                                width: parent.width - Math.floor((parent.width - 1) / 2) - 1
                                height: 3
                                label: "[X] STOP"
                                normalColor: "#3a1e1e"
                                focusColor: "#5a2e2e"
                                textColor: HmiTheme.textDanger
                                onClicked: triggerStop()
                            }
                        }

                        Row {
                            width: parent.width
                            height: 3
                            spacing: 1
                            Button {
                                width: Math.floor((parent.width - 1) / 2)
                                height: 3
                                label: "[R] RESET"
                                normalColor: "#3a301e"
                                focusColor: "#5a482e"
                                textColor: HmiTheme.textWarning
                                onClicked: requestReset()
                            }
                            Button {
                                width: parent.width - Math.floor((parent.width - 1) / 2) - 1
                                height: 3
                                label: "[M] MAINT"
                                normalColor: "#21252b"
                                focusColor: "#2c313a"
                                textColor: HmiTheme.textHighlight
                                onClicked: requestMaintenance()
                            }
                        }
                    }
                }
            }

            // VIEW 1: ALARMS & LOGS
            AlarmsView {
                visible: rootWindow.activeTab === 1
                logs: rootWindow.logMessages
                errorCount: rootWindow.errorCount
            }

            // VIEW 2: HARDWARE DIAGNOSTICS
            DiagnosticsView {
                visible: rootWindow.activeTab === 2
                cycles: rootWindow.cycles
                valveA: rootWindow.valveA
                valveB: rootWindow.valveB
                machineState: rootWindow.machineState
            }

            // VIEW 3: SYSTEM PARAMETERS & SETTINGS
            SettingsView {
                visible: rootWindow.activeTab === 3
                targetSpeed: rootWindow.targetSpeed
                maxTempLimit: rootWindow.maxTempLimit
                onSpeedChanged: (newSpeed) => {
                    rootWindow.targetSpeed = newSpeed;
                    rootWindow.log("SET RPM -> " + newSpeed);
                }
                onTempLimitChanged: (newLimit) => {
                    rootWindow.maxTempLimit = newLimit;
                    rootWindow.log("LIMIT TEMP -> " + newLimit + "C");
                }
            }
        }

        // 4. FOOTER ALARM / LOG TICKER & NAVIGATION KEYMAP
        Rectangle {
            id: footerPanel
            height: 4
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            border.style: "single"
            border.color: rootWindow.machineState === "error" ? HmiTheme.borderError : HmiTheme.borderNormal
            color: rootWindow.machineState === "error" ? HmiTheme.footerBgError : HmiTheme.footerBg

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0

                Text {
                    width: parent.width
                    text: rootWindow.machineState === "error"
                          ? "🚨 CRITICAL FAULT: OVER-TEMP LIMIT (" + rootWindow.temperature.toFixed(1) + "°C >= " + rootWindow.maxTempLimit.toFixed(1) + "°C)"
                          : "STATUS: " + rootWindow.logMessages[0]
                    color: rootWindow.machineState === "error" ? HmiTheme.textWarning : HmiTheme.textPrimary
                    bold: true
                }

                Text {
                    width: parent.width
                    text: rootWindow.machineState === "error"
                          ? "ACTION: PRESS [R] / RESET BUTTON TO CLEAR ALARM LOCKOUT"
                          : "KEYS: [F1-F4] Views  [S]Start  [X]Stop  [R]Reset  [M]Maint  [Q]Quit"
                    color: rootWindow.machineState === "error" ? HmiTheme.textDanger : HmiTheme.textSecondary
                    bold: rootWindow.machineState === "error"
                }
            }
        }

        // 5. MODAL SAFETY INTERLOCK DIALOG
        ConfirmDialog {
            id: confirmDlg
            onConfirmed: executeConfirmedAction()
            onCancelled: rootWindow.pendingAction = ""
        }
    }
}
