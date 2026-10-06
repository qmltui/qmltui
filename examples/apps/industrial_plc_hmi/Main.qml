import QtQml
import QtQml.Models
import Qtui
import Qtui.Controls
import IndustrialPlcHmi

Terminal {
    id: rootWindow
    color: "black"

    property int activeTab: PlcTypes.Dashboard
    property int machineState: PlcTypes.Idle
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
    property int pendingAction: PlcTypes.NoAction
    property string latestStatus: "READY FOR OPERATOR COMMANDS"
    property int nextEventSequence: 4
    property bool deterministicSimulation: false
    property int deterministicNoiseIndex: 0
    property list<real> deterministicNoise: [0.25, 0.75, 0.5, 0.9, 0.1, 0.6]
    property list<real> tempHistory: [28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5, 28.5]
    property list<real> speedHistory: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    property list<real> pressureHistory: [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]

    ListModel {
        id: eventLog

        ListElement { sequence: 1; severity: "INFO"; message: "SYSTEM INITIALIZED" }
        ListElement { sequence: 2; severity: "INFO"; message: "STATE CHANGED TO: IDLE" }
        ListElement { sequence: 3; severity: "INFO"; message: "READY FOR OPERATOR COMMANDS" }
    }

    onMachineStateChanged: {
        if (!rootWindow.isKnownMachineState(rootWindow.machineState)) {
            rootWindow.enterFault("INVALID PLC STATE DETECTED")
        }
    }

    // Mandatory Quit key handler as per workspace rule + Navigation & Control Shortcuts
    Keys.onPressed: (event) => {
        if (confirmDlg.visible) {
            if (event.key === Qt.Key_Y) {
                confirmDlg.accept()
            } else if (event.key === Qt.Key_N || event.key === Qt.Key_Escape) {
                confirmDlg.reject()
            }
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return;
        }

        // View Tab Switching Shortcuts
        if (event.key === Qt.Key_F1 || event.key === Qt.Key_1) {
            activeTab = PlcTypes.Dashboard;
            event.accepted = true;
            return;
        } else if (event.key === Qt.Key_F2 || event.key === Qt.Key_2) {
            activeTab = PlcTypes.Alarms;
            event.accepted = true;
            return;
        } else if (event.key === Qt.Key_F3 || event.key === Qt.Key_3) {
            activeTab = PlcTypes.Diagnostics;
            event.accepted = true;
            return;
        } else if (event.key === Qt.Key_F4 || event.key === Qt.Key_4) {
            activeTab = PlcTypes.Settings;
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

    function logEvent(severity, message) {
        eventLog.append({
            "sequence": rootWindow.nextEventSequence,
            "severity": severity,
            "message": message
        })
        rootWindow.nextEventSequence += 1
        if (eventLog.count > 10) {
            eventLog.remove(0, eventLog.count - 10)
        }
        rootWindow.latestStatus = message
    }

    function pushHistory(newTemp, newSpeed, newPressure) {
        const nextTemperatures = tempHistory.slice(1)
        nextTemperatures.push(newTemp)
        tempHistory = nextTemperatures

        const nextSpeeds = speedHistory.slice(1)
        nextSpeeds.push(newSpeed)
        speedHistory = nextSpeeds

        const nextPressures = pressureHistory.slice(1)
        nextPressures.push(newPressure)
        pressureHistory = nextPressures
    }

    function simulationNoise(minimum, maximum) {
        let unitValue = Math.random()
        if (rootWindow.deterministicSimulation) {
            unitValue = rootWindow.deterministicNoise[
                        rootWindow.deterministicNoiseIndex % rootWindow.deterministicNoise.length]
            rootWindow.deterministicNoiseIndex += 1
        }
        return minimum + unitValue * (maximum - minimum)
    }

    function isKnownMachineState(state) {
        return state === PlcTypes.Idle
                || state === PlcTypes.Processing
                || state === PlcTypes.Error
                || state === PlcTypes.Maintenance
    }

    function setTargetSpeed(input) {
        const newSpeed = Number(input)
        if (!Number.isFinite(newSpeed) || !Number.isInteger(newSpeed)
                || newSpeed < 0 || newSpeed > 3000) {
            logEvent("WARNING", "REJECTED RPM SETPOINT: " + input)
            return false
        }
        targetSpeed = newSpeed
        logEvent("INFO", "SET RPM -> " + newSpeed)
        return true
    }

    function setMaxTempLimit(input) {
        const newLimit = Number(input)
        if (!Number.isFinite(newLimit) || newLimit < 30.0 || newLimit > 110.0) {
            logEvent("WARNING", "REJECTED TEMP LIMIT: " + input)
            return false
        }
        maxTempLimit = newLimit
        logEvent("INFO", "LIMIT TEMP -> " + newLimit.toFixed(1) + "C")
        return true
    }

    function enterFault(message) {
        machineState = PlcTypes.Error
        speed = 0
        valveA = false
        valveB = false
        pendingAction = PlcTypes.NoAction
        errorCount += 1
        logEvent("CRITICAL", message)
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
        if (machineState !== PlcTypes.Error) {
            machineState = PlcTypes.Processing
            logEvent("INFO", "STATE: PROCESSING (START CMD)")
        }
    }

    function triggerStop() {
        if (machineState !== PlcTypes.Error) {
            machineState = PlcTypes.Idle
            valveA = false
            valveB = false
            logEvent("INFO", "STATE: IDLE (STOP CMD)")
        }
    }

    function requestReset() {
        if (machineState === PlcTypes.Error) {
            pendingAction = PlcTypes.Reset
            confirmDlg.open("CLEAR FAULT ALARM",
                            "Acknowledge and reset over-temperature alarm lockout?")
        }
    }

    function requestMaintenance() {
        if (machineState !== PlcTypes.Error) {
            pendingAction = PlcTypes.ToggleMaintenance
            confirmDlg.open("MAINTENANCE MODE",
                            machineState === PlcTypes.Maintenance
                            ? "Exit maintenance mode and return station to IDLE?"
                            : "Switch PLC station into low-speed MAINTENANCE mode?")
        }
    }

    function executeConfirmedAction() {
        if (pendingAction === PlcTypes.Reset) {
            machineState = PlcTypes.Idle
            temperature = 35.0
            logEvent("INFO", "STATE: IDLE (ALARM RESET)")
        } else if (pendingAction === PlcTypes.ToggleMaintenance) {
            if (machineState === PlcTypes.Maintenance) {
                machineState = PlcTypes.Idle
                valveA = false
                valveB = false
                logEvent("INFO", "STATE: IDLE (MAINT RUN COMPLETED)")
            } else {
                machineState = PlcTypes.Maintenance
                speed = 150
                valveA = true
                valveB = false
                logEvent("INFO", "STATE: MAINTENANCE ACTIVE")
            }
        }
        pendingAction = PlcTypes.NoAction
    }

    // Telemetry & States Simulation Timer
    Timer {
        id: simTimer
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            if (rootWindow.machineState === PlcTypes.Processing) {
                rootWindow.cycles += 1;
                
                // Ramp speed up towards setpoint
                if (rootWindow.speed < rootWindow.targetSpeed) {
                    rootWindow.speed = Math.min(rootWindow.targetSpeed, rootWindow.speed + 200);
                } else if (rootWindow.speed > rootWindow.targetSpeed) {
                    rootWindow.speed = Math.max(rootWindow.targetSpeed, rootWindow.speed - 200);
                }
                
                // Temp goes up based on speed
                let tempFactor = (rootWindow.speed / 1000.0) * 1.8;
                rootWindow.temperature = Math.min(
                            110.0,
                            rootWindow.temperature + tempFactor
                            + rootWindow.simulationNoise(-0.5, 1.0))
                
                // Pressure fluctuates relative to temp
                rootWindow.pressure = 1.0
                        + (rootWindow.temperature - 25.0) * 0.06
                        + rootWindow.simulationNoise(-0.05, 0.05)
                
                // Valve simulation sequence
                rootWindow.valveA = (rootWindow.cycles % 8 < 4);
                rootWindow.valveB = (rootWindow.cycles % 8 >= 4);

                // Check safety limit
                if (rootWindow.temperature >= rootWindow.maxTempLimit) {
                    rootWindow.enterFault("OVER-TEMPERATURE FAULT")
                }
            } else if (rootWindow.machineState === PlcTypes.Idle
                       || rootWindow.machineState === PlcTypes.Error) {
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
            } else if (rootWindow.machineState === PlcTypes.Maintenance) {
                // Maintenance low speed check
                rootWindow.speed = 150;
                rootWindow.temperature = 31.0;
                rootWindow.pressure = 1.1;
                rootWindow.valveA = true;
                rootWindow.valveB = false;
            }

            // Periodic operational logging
            if (rootWindow.machineState === PlcTypes.Processing
                    && rootWindow.cycles % 5 === 0) {
                rootWindow.logEvent(
                            "INFO",
                            "PROCESS CYCLE " + rootWindow.cycles + " | T: "
                            + rootWindow.temperature.toFixed(1) + "C")
            }

            // Push to telemetry history
            rootWindow.pushHistory(rootWindow.temperature, rootWindow.speed, rootWindow.pressure);

            // Update Braille Plot points
            if (rootWindow.activeTab === PlcTypes.Dashboard
                    && tempCanvas.width > 0 && tempCanvas.height > 0) {
                tempCanvas.updatePlot();
            }
        }
    }

    // Alarm screen blinking timer
    Timer {
        id: blinkTimer
        interval: 500
        running: rootWindow.machineState === PlcTypes.Error
        repeat: true
        onTriggered: {
            rootWindow.alarmBlinking = !rootWindow.alarmBlinking;
        }
    }

    // Main layout wrapper
    Rectangle {
        id: mainRect
        anchors.fill: parent
        color: rootWindow.machineState === PlcTypes.Error
               ? (rootWindow.alarmBlinking ? HmiTheme.bgError : HmiTheme.bgErrorAlt)
               : (rootWindow.machineState === PlcTypes.Maintenance
                  ? HmiTheme.bgMaintenance
                  : (rootWindow.machineState === PlcTypes.Processing
                     ? HmiTheme.bgProcessing : HmiTheme.bgIdle))

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
        Item {
            id: viewContainer
            anchors.top: navBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: footerPanel.top

            // VIEW 0: MAIN DASHBOARD VIEW
            Item {
                anchors.fill: parent
                visible: rootWindow.activeTab === PlcTypes.Dashboard

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
                        spacing: 0

                        Text {
                            text: "📊 TELEMETRY & GAUGES"
                            color: HmiTheme.textSecondary
                            bold: true
                        }

                        Gauge {
                            label: "Speed"
                            value: rootWindow.speed
                            unit: "RPM"
                            from: 0
                            to: 3000
                            warningVal: 2500
                            dangerVal: 2800
                            decimals: 0
                        }
                        Sparkline {
                            width: parent.width
                            height: 1
                            values: rootWindow.speedHistory
                            from: 0
                            to: 3000
                            color: HmiTheme.statusProcessing
                        }

                        Gauge {
                            label: "Temp"
                            value: rootWindow.temperature
                            unit: "°C"
                            from: 20
                            to: 100
                            warningVal: 75
                            dangerVal: rootWindow.maxTempLimit
                            decimals: 1
                        }
                        Sparkline {
                            width: parent.width
                            height: 1
                            values: rootWindow.tempHistory
                            from: 20
                            to: 100
                            color: rootWindow.temperature >= rootWindow.maxTempLimit ? HmiTheme.statusError : HmiTheme.textWarning
                        }

                        Gauge {
                            label: "Pres"
                            value: rootWindow.pressure
                            unit: "bar"
                            from: 0
                            to: 5
                            warningVal: 3.5
                            dangerVal: 4.5
                            decimals: 2
                        }
                        Sparkline {
                            width: parent.width
                            height: 1
                            values: rootWindow.pressureHistory
                            from: 0
                            to: 5
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

                        Item {
                            id: conveyorPulseRect
                            property int pulseFrame: 0
                            readonly property list<string> flowFrames: [">  ", ">> ", ">>>", " >>", "  >", "   "]

                            width: parent.width
                            height: 1

                            Timer {
                                interval: rootWindow.machineState === PlcTypes.Maintenance ? 500 : 125
                                running: rootWindow.activeTab === PlcTypes.Dashboard
                                         && (rootWindow.machineState === PlcTypes.Processing
                                             || rootWindow.machineState === PlcTypes.Maintenance)
                                repeat: true
                                onTriggered: conveyorPulseRect.pulseFrame += 1
                            }

                            Text {
                                anchors.centerIn: parent
                                text: {
                                    if (rootWindow.machineState === PlcTypes.Processing) {
                                        return "[ FLOW: " + conveyorPulseRect.flowFrames[
                                                    conveyorPulseRect.pulseFrame
                                                    % conveyorPulseRect.flowFrames.length] + " ]"
                                    } else if (rootWindow.machineState === PlcTypes.Maintenance) {
                                        return "[ MAINT: " + (conveyorPulseRect.pulseFrame % 2 === 0
                                                               ? "◧" : "◨") + " ]"
                                    } else if (rootWindow.machineState === PlcTypes.Error) {
                                        return "[ 🛑 LOCKED ]";
                                    } else {
                                        return "[ ■ IDLE ]";
                                    }
                                }
                                color: rootWindow.machineState === PlcTypes.Processing ? HmiTheme.textSuccess : (rootWindow.machineState === PlcTypes.Error ? HmiTheme.textDanger : HmiTheme.textDim)
                                bold: true
                            }
                        }

                        Text {
                            text: "Cycles: " + rootWindow.cycles
                                  + " | Alarms: " + rootWindow.errorCount
                            color: rootWindow.errorCount > 0
                                   ? HmiTheme.textDanger : HmiTheme.textPrimary
                            bold: rootWindow.errorCount > 0
                        }
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
                spacing: 0

                        Text {
                            text: "📈 TEMP TREND (20-100°C)"
                            color: HmiTheme.textSecondary
                            bold: true
                        }

                        Text {
                            text: "CUR: " + rootWindow.temperature.toFixed(1) + "°C | MAX: " + rootWindow.maxTempLimit.toFixed(1) + "°C"
                            color: rootWindow.temperature >= rootWindow.maxTempLimit ? HmiTheme.textDanger : HmiTheme.textHighlight
                        }

                Item {
                    width: parent.width
                    height: Math.max(3, parent.height - 3)

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
                            color: rootWindow.machineState === PlcTypes.Error
                                   ? HmiTheme.statusError
                                   : (rootWindow.machineState === PlcTypes.Processing
                                      ? HmiTheme.textHighlight : HmiTheme.statusIdle)

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
                spacing: 0

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
                            rootWindow.setTargetSpeed(rpmInput.text)
                            rpmInput.text = rootWindow.targetSpeed.toString()
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
                            rootWindow.setMaxTempLimit(tempLimitInput.text)
                            tempLimitInput.text = rootWindow.maxTempLimit.toString()
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
                                text: "[S] START"
                                color: "#1e3a1e"
                                textColor: HmiTheme.textSuccess
                        onClicked: rootWindow.triggerStart()
                            }
                            Button {
                                width: parent.width - Math.floor((parent.width - 1) / 2) - 1
                                height: 3
                                text: "[X] STOP"
                                color: "#3a1e1e"
                                textColor: HmiTheme.textDanger
                        onClicked: rootWindow.triggerStop()
                            }
                        }

                        Row {
                            width: parent.width
                            height: 3
                            spacing: 1
                            Button {
                                width: Math.floor((parent.width - 1) / 2)
                                height: 3
                                text: "[R] RESET"
                                color: "#3a301e"
                                textColor: HmiTheme.textWarning
                        onClicked: rootWindow.requestReset()
                            }
                            Button {
                                width: parent.width - Math.floor((parent.width - 1) / 2) - 1
                                height: 3
                                text: "[M] MAINT"
                                color: "#21252b"
                                textColor: HmiTheme.textHighlight
                        onClicked: rootWindow.requestMaintenance()
                            }
                        }
                    }
                }
            }

            // VIEW 1: ALARMS & LOGS
            AlarmsView {
                visible: rootWindow.activeTab === PlcTypes.Alarms
                eventModel: eventLog
                errorCount: rootWindow.errorCount
            }

            // VIEW 2: HARDWARE DIAGNOSTICS
            DiagnosticsView {
                visible: rootWindow.activeTab === PlcTypes.Diagnostics
                cycles: rootWindow.cycles
                valveA: rootWindow.valveA
                valveB: rootWindow.valveB
                machineState: rootWindow.machineState
            }

            // VIEW 3: SYSTEM PARAMETERS & SETTINGS
            SettingsView {
                visible: rootWindow.activeTab === PlcTypes.Settings
                targetSpeed: rootWindow.targetSpeed
                maxTempLimit: rootWindow.maxTempLimit
                onTargetSpeedRequested: (input) => rootWindow.setTargetSpeed(input)
                onTempLimitRequested: (input) => rootWindow.setMaxTempLimit(input)
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
            border.color: rootWindow.machineState === PlcTypes.Error ? HmiTheme.borderError : HmiTheme.borderNormal
            color: rootWindow.machineState === PlcTypes.Error ? HmiTheme.footerBgError : HmiTheme.footerBg

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0

                Text {
                    width: parent.width
                    text: rootWindow.machineState === PlcTypes.Error
                          ? "🚨 CRITICAL FAULT: OVER-TEMP LIMIT (" + rootWindow.temperature.toFixed(1) + "°C >= " + rootWindow.maxTempLimit.toFixed(1) + "°C)"
                          : "STATUS: " + rootWindow.latestStatus
                    color: rootWindow.machineState === PlcTypes.Error ? HmiTheme.textWarning : HmiTheme.textPrimary
                    bold: true
                }

                Text {
                    width: parent.width
                    text: rootWindow.machineState === PlcTypes.Error
                          ? "ACTION: PRESS [R] / RESET BUTTON TO CLEAR ALARM LOCKOUT"
                          : "KEYS: [F1-F4] Views  [S]Start  [X]Stop  [R]Reset  [M]Maint  [Q]Quit"
                    color: rootWindow.machineState === PlcTypes.Error ? HmiTheme.textDanger : HmiTheme.textSecondary
                    bold: rootWindow.machineState === PlcTypes.Error
                }
            }
        }

        // 5. MODAL SAFETY INTERLOCK DIALOG
        ConfirmDialog {
            id: confirmDlg
            onConfirmed: rootWindow.executeConfirmedAction()
            onCancelled: rootWindow.pendingAction = PlcTypes.NoAction
        }
    }
}
