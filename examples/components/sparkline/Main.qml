import Qtui.Extras
import Qtui.Controls
pragma ComponentBehavior: Bound

import QtQml
import Qtui

Terminal {
    id: rootWindow
    fillTerminal: true
    color: "black"
    useAlternateScreen: true

    // Maximum history data points for sparklines
    property int maxHistory: 100

    // Data History Arrays
    property var cpu0History: []
    property var cpu1History: []
    property var cpu2History: []
    property var cpu3History: []

    property var netRxHistory: []
    property var netTxHistory: []

    property var memHistory: []
    property var swapHistory: []

    property var diskReadHistory: []
    property var diskWriteHistory: []

    property var reqRateHistory: []
    property var latencyHistory: []

    property var tempHistory: []

    // State Variables
    property bool running: true
    property int stepCount: 0
    property int uptimeSeconds: 0
    property int timerInterval: 150

    // Simulation Surge Countdowns
    property int loadSpikeTicks: 0
    property int memSurgeTicks: 0
    property int netBurstTicks: 0

    // Current Values
    property double cpu0Val: 25.0
    property double cpu1Val: 40.0
    property double cpu2Val: 15.0
    property double cpu3Val: 30.0

    property double netRxVal: 12.5
    property double netTxVal: 8.2

    property double memVal: 45.0
    property double swapVal: 10.0

    property double diskReadVal: 350.0
    property double diskWriteVal: 180.0

    property double reqRateVal: 1200.0
    property double latencyVal: 35.0

    property double tempVal: 42.0

    // Summary Stats
    property double avgCpu: 27.5
    property double peakLatency: 35.0
    property int warningCount: 0
    property int critCount: 0

    // Event Log
    property var logMessages: []

    // Explicit Key Handling for Quitting and Actions (rule requirement)
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Space) {
            rootWindow.running = !rootWindow.running
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_S) {
            rootWindow.loadSpikeTicks = 20
            rootWindow.addLog("WARN", "Manual load spike triggered via [S]")
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_L) {
            rootWindow.memSurgeTicks = 25
            rootWindow.addLog("WARN", "Memory surge simulation triggered via [L]")
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_N) {
            rootWindow.netBurstTicks = 20
            rootWindow.addLog("INFO", "Network traffic burst triggered via [N]")
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_R) {
            rootWindow.resetAll()
            rootWindow.addLog("INFO", "Telemetry buffers and stats reset via [R]")
            event.accepted = true
            return
        }
    }

    function formatUptime(seconds) {
        let hrs = Math.floor(seconds / 3600);
        let mins = Math.floor((seconds % 3600) / 60);
        let secs = seconds % 60;
        return (hrs < 10 ? "0" + hrs : hrs) + ":" +
               (mins < 10 ? "0" + mins : mins) + ":" +
               (secs < 10 ? "0" + secs : secs);
    }

    function addLog(level, msg) {
        let timeStr = formatUptime(uptimeSeconds);
        let line = "[" + timeStr + "] " + level + " - " + msg;
        let logs = Array.from(logMessages);
        if (logs.length >= 5) logs.shift();
        logs.push(line);
        logMessages = logs;
    }

    function appendValue(historyArray, newValue) {
        let arr = Array.from(historyArray);
        if (arr.length >= maxHistory) {
            arr.shift();
        }
        arr.push(newValue);
        return arr;
    }

    function initData() {
        let c0 = [], c1 = [], c2 = [], c3 = [];
        let rx = [], tx = [], mem = [], swp = [];
        let dr = [], dw = [], req = [], lat = [], tmp = [];

        for (let i = 0; i < maxHistory; i++) {
            c0.push(20 + Math.sin(i * 0.15) * 10);
            c1.push(35 + Math.cos(i * 0.2) * 15);
            c2.push(15 + Math.sin(i * 0.25) * 8);
            c3.push(25 + Math.cos(i * 0.1) * 12);

            rx.push(10 + Math.random() * 5);
            tx.push(5 + Math.random() * 3);

            mem.push(40 + (i * 0.2) % 40);
            swp.push(8 + Math.random() * 2);

            dr.push(200 + Math.random() * 100);
            dw.push(100 + Math.random() * 50);

            req.push(1000 + Math.random() * 300);
            lat.push(25 + Math.random() * 10);

            tmp.push(40 + Math.random() * 3);
        }

        cpu0History = c0; cpu1History = c1; cpu2History = c2; cpu3History = c3;
        netRxHistory = rx; netTxHistory = tx;
        memHistory = mem; swapHistory = swp;
        diskReadHistory = dr; diskWriteHistory = dw;
        reqRateHistory = req; latencyHistory = lat;
        tempHistory = tmp;

        logMessages = [
            "[00:00:00] INFO - Telemetry monitoring engine initialized",
            "[00:00:01] INFO - Connected to cluster node host-prod-01"
        ];
    }

    function resetAll() {
        stepCount = 0;
        uptimeSeconds = 0;
        peakLatency = 35.0;
        loadSpikeTicks = 0;
        memSurgeTicks = 0;
        netBurstTicks = 0;
        initData();
    }

    function cycleSpeed() {
        if (timerInterval === 150) timerInterval = 50;
        else if (timerInterval === 50) timerInterval = 300;
        else if (timerInterval === 300) timerInterval = 500;
        else timerInterval = 150;
    }

    function tickSimulation() {
        stepCount++;
        if (stepCount % Math.floor(1000 / timerInterval) === 0) {
            uptimeSeconds++;
        }

        // Noise base
        let noise0 = (Math.random() - 0.5) * 8;
        let noise1 = (Math.random() - 0.5) * 10;
        let noise2 = (Math.random() - 0.5) * 6;
        let noise3 = (Math.random() - 0.5) * 9;

        // Base CPU patterns
        let base0 = 28 + Math.sin(stepCount * 0.15) * 15;
        let base1 = 42 + Math.cos(stepCount * 0.2) * 20;
        let base2 = 18 + Math.sin(stepCount * 0.25) * 10;
        let base3 = 32 + Math.cos(stepCount * 0.1) * 15;

        if (loadSpikeTicks > 0) {
            loadSpikeTicks--;
            base0 += 40 + Math.random() * 15;
            base1 += 45 + Math.random() * 10;
            base2 += 35 + Math.random() * 20;
            base3 += 50 + Math.random() * 10;
        }

        cpu0Val = Math.max(2, Math.min(100, Math.round(base0 + noise0)));
        cpu1Val = Math.max(2, Math.min(100, Math.round(base1 + noise1)));
        cpu2Val = Math.max(2, Math.min(100, Math.round(base2 + noise2)));
        cpu3Val = Math.max(2, Math.min(100, Math.round(base3 + noise3)));

        avgCpu = (cpu0Val + cpu1Val + cpu2Val + cpu3Val) / 4.0;

        // Memory
        let memBase = memVal;
        if (memSurgeTicks > 0) {
            memSurgeTicks--;
            memBase += 2.5;
            if (memBase >= 92.0) {
                memBase = 41.0;
                addLog("WARN", "Garbage collection reclaimed 51% RAM");
            }
        } else {
            memBase += (Math.random() - 0.48) * 0.5;
            memBase = Math.max(30.0, Math.min(85.0, memBase));
        }
        memVal = Math.round(memBase * 10) / 10.0;
        swapVal = Math.round((8.0 + (memVal > 70 ? (memVal - 70) * 0.5 : 0) + Math.random()) * 10) / 10.0;

        // Network
        let rxBase = 12.0 + Math.sin(stepCount * 0.3) * 6.0 + (Math.random() - 0.5) * 4.0;
        let txBase = 7.0 + Math.cos(stepCount * 0.3) * 4.0 + (Math.random() - 0.5) * 3.0;

        if (netBurstTicks > 0) {
            netBurstTicks--;
            rxBase += 35.0 + Math.random() * 10.0;
            txBase += 25.0 + Math.random() * 8.0;
        }
        if (loadSpikeTicks > 0) {
            rxBase += 20.0;
            txBase += 15.0;
        }

        netRxVal = Math.max(0.5, Math.round(rxBase * 10) / 10.0);
        netTxVal = Math.max(0.2, Math.round(txBase * 10) / 10.0);

        // Storage IOPS
        let drBase = 300 + (loadSpikeTicks > 0 ? 500 : 0) + Math.random() * 200;
        let dwBase = 150 + (loadSpikeTicks > 0 ? 300 : 0) + Math.random() * 100;
        diskReadVal = Math.round(drBase);
        diskWriteVal = Math.round(dwBase);

        // Req Rate & Latency
        let reqBase = 1200 + (loadSpikeTicks > 0 ? 1500 : 0) + Math.sin(stepCount * 0.2) * 400 + Math.random() * 200;
        let latBase = 28 + (loadSpikeTicks > 0 ? 95 : 0) + (cpu1Val > 80 ? (cpu1Val - 80) * 2 : 0) + Math.random() * 12;

        reqRateVal = Math.round(reqBase);
        latencyVal = Math.round(latBase);

        if (latencyVal > peakLatency) {
            peakLatency = latencyVal;
        }

        // Temperature
        let tempBase = 38.0 + (avgCpu / 100.0) * 25.0 + Math.random() * 1.5;
        tempVal = Math.round(tempBase * 10) / 10.0;

        // Alerts count
        let wCount = 0;
        let cCount = 0;
        if (cpu1Val > 85 || cpu0Val > 85) wCount++;
        if (memVal > 80) wCount++;
        if (latencyVal > 100) cCount++;
        if (tempVal > 65) cCount++;
        warningCount = wCount;
        critCount = cCount;

        // Periodic event logging
        if (stepCount % 60 === 0 && loadSpikeTicks === 0) {
            addLog("INFO", "System health check OK - Avg CPU: " + Math.round(avgCpu) + "%");
        }

        // Push values to history arrays
        cpu0History = appendValue(cpu0History, cpu0Val);
        cpu1History = appendValue(cpu1History, cpu1Val);
        cpu2History = appendValue(cpu2History, cpu2Val);
        cpu3History = appendValue(cpu3History, cpu3Val);

        netRxHistory = appendValue(netRxHistory, netRxVal);
        netTxHistory = appendValue(netTxHistory, netTxVal);

        memHistory = appendValue(memHistory, memVal);
        swapHistory = appendValue(swapHistory, swapVal);

        diskReadHistory = appendValue(diskReadHistory, diskReadVal);
        diskWriteHistory = appendValue(diskWriteHistory, diskWriteVal);

        reqRateHistory = appendValue(reqRateHistory, reqRateVal);
        latencyHistory = appendValue(latencyHistory, latencyVal);

        tempHistory = appendValue(tempHistory, tempVal);
    }

    Timer {
        id: telemetryTimer
        interval: rootWindow.timerInterval
        running: rootWindow.running
        repeat: true
        onTriggered: rootWindow.tickSimulation()
    }

    Component.onCompleted: {
        rootWindow.initData()
    }

    // Main Layout Outer Frame
    Rectangle {
        anchors.fill: parent
        color: "#0c0d10"

        // 1. HEADER PANEL
        Rectangle {
            id: header
            height: 3
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            border.style: "double"
            border.color: "cyan"
            color: "#141b24"

            Row {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 2

                Text {
                    text: " 📡 REAL-TIME CLUSTER & TELEMETRY MONITOR"
                    color: "cyan"
                    bold: true
                }

                Text {
                    text: rootWindow.running ? "[🟢 LIVE]" : "[⏸ PAUSED]"
                    color: rootWindow.running ? "green" : "yellow"
                    bold: true
                }

                Text {
                    text: "Uptime: " + rootWindow.formatUptime(rootWindow.uptimeSeconds) + " | Ticks: " + rootWindow.stepCount
                    color: "gray"
                }
            }
        }

        // 2. FOOTER PANEL
        Rectangle {
            id: footer
            height: 3
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            border.style: "single"
            border.color: "cyan"
            color: "#0d1117"

            Text {
                anchors.centerIn: parent
                text: "[Space] Pause/Resume  |  [S] Load Spike  |  [L] Mem Surge  |  [N] Net Burst  |  [R] Reset  |  [Q/Esc] Quit"
                color: "gray"
            }
        }

        // 3. SIDEBAR CONTROLS & SUMMARY (LEFT)
        Rectangle {
            id: sidebar
            width: 36
            anchors.top: header.bottom
            anchors.bottom: footer.top
            anchors.left: parent.left
            border.style: "single"
            border.color: "magenta"
            color: "#12141c"

            Column {
                id: sidebarColumn
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                Text {
                    width: sidebarColumn.width
                    text: "🎛️ CONTROLS & SIMULATION"
                    color: "magenta"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                }

                Button {
                    width: sidebarColumn.width
                    height: 3
                    normalBorderStyle: "none"
                    focusBorderStyle: "single"
                    normalColor: "#1d212a"
                    focusColor: "#323947"
                    text: rootWindow.running ? "⏸ PAUSE MONITOR" : "▶ RESUME MONITOR"
                    onClicked: rootWindow.running = !rootWindow.running
                }

                Row {
                    width: sidebarColumn.width
                    height: 3
                    spacing: 1
                    Button {
                        width: Math.floor((sidebarColumn.width - 1) / 2)
                        height: 3
                        normalBorderStyle: "none"
                        focusBorderStyle: "single"
                        normalColor: "#1d212a"
                        focusColor: "#323947"
                        text: "⚡ SPIKE [S]"
                        onClicked: {
                            rootWindow.loadSpikeTicks = 20;
                            rootWindow.addLog("WARN", "Manual load spike triggered");
                        }
                    }
                    Button {
                        width: sidebarColumn.width - Math.floor((sidebarColumn.width - 1) / 2) - 1
                        height: 3
                        normalBorderStyle: "none"
                        focusBorderStyle: "single"
                        normalColor: "#1d212a"
                        focusColor: "#323947"
                        text: "🌊 MEM [L]"
                        onClicked: {
                            rootWindow.memSurgeTicks = 25;
                            rootWindow.addLog("WARN", "Memory surge triggered");
                        }
                    }
                }

                Row {
                    width: sidebarColumn.width
                    height: 3
                    spacing: 1
                    Button {
                        width: Math.floor((sidebarColumn.width - 1) / 2)
                        height: 3
                        normalBorderStyle: "none"
                        focusBorderStyle: "single"
                        normalColor: "#1d212a"
                        focusColor: "#323947"
                        text: "🚀 NET [N]"
                        onClicked: {
                            rootWindow.netBurstTicks = 20;
                            rootWindow.addLog("INFO", "Traffic burst triggered");
                        }
                    }
                    Button {
                        width: sidebarColumn.width - Math.floor((sidebarColumn.width - 1) / 2) - 1
                        height: 3
                        normalBorderStyle: "none"
                        focusBorderStyle: "single"
                        normalColor: "#1d212a"
                        focusColor: "#323947"
                        text: "🔄 RESET [R]"
                        onClicked: rootWindow.resetAll()
                    }
                }

                Button {
                    width: sidebarColumn.width
                    height: 3
                    normalBorderStyle: "none"
                    focusBorderStyle: "single"
                    normalColor: "#1d212a"
                    focusColor: "#323947"
                    text: "⏱️ INTERVAL: " + rootWindow.timerInterval + "ms"
                    onClicked: rootWindow.cycleSpeed()
                }

                Text {
                    width: sidebarColumn.width
                    text: "──────────────────────────────────"
                    color: "darkgray"
                }

                Text {
                    width: sidebarColumn.width
                    text: "📊 CLUSTER SUMMARY"
                    color: "cyan"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                }

                Text {
                    width: sidebarColumn.width
                    text: " • Avg CPU Load : " + Math.round(rootWindow.avgCpu) + "%"
                    color: rootWindow.avgCpu > 70 ? "red" : (rootWindow.avgCpu > 50 ? "yellow" : "green")
                }

                Text {
                    width: sidebarColumn.width
                    text: " • RAM Allocation: " + rootWindow.memVal + "%"
                    color: rootWindow.memVal > 80 ? "red" : "silver"
                }

                Text {
                    width: sidebarColumn.width
                    text: " • Net In/Out   : " + rootWindow.netRxVal + " / " + rootWindow.netTxVal + " MB/s"
                    color: "silver"
                }

                Text {
                    width: sidebarColumn.width
                    text: " • p99 Latency  : " + rootWindow.latencyVal + " ms"
                    color: rootWindow.latencyVal > 80 ? "red" : "silver"
                }

                Text {
                    width: sidebarColumn.width
                    text: " • Peak Latency : " + rootWindow.peakLatency + " ms"
                    color: "gray"
                }

                Text {
                    width: sidebarColumn.width
                    text: " • Status Alerts: " + (rootWindow.critCount > 0 ? rootWindow.critCount + " CRIT " : "") + (rootWindow.warningCount > 0 ? rootWindow.warningCount + " WARN" : (rootWindow.critCount === 0 ? "0 OK" : ""))
                    color: rootWindow.critCount > 0 ? "red" : (rootWindow.warningCount > 0 ? "yellow" : "green")
                    bold: true
                }

                Text {
                    width: sidebarColumn.width
                    text: "──────────────────────────────────"
                    color: "darkgray"
                }

                Text {
                    width: sidebarColumn.width
                    text: "📋 LIVE EVENT LOG"
                    color: "yellow"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                }

                Repeater {
                    model: rootWindow.logMessages
                    delegate: Text {
                        required property string modelData
                        width: sidebarColumn.width
                        text: modelData
                        color: modelData.indexOf("WARN") >= 0 ? "yellow" : (modelData.indexOf("CRIT") >= 0 ? "red" : "gray")
                    }
                }
            }
        }

        // 4. MAIN METRICS DISPLAY PANELS (RIGHT)
        Rectangle {
            id: mainContent
            anchors.top: header.bottom
            anchors.bottom: footer.top
            anchors.left: sidebar.right
            anchors.right: parent.right
            color: "transparent"

            Column {
                anchors.fill: parent
                spacing: 0

                // PANEL 1: CPU MULTI-CORE UTILIZATION
                Rectangle {
                    width: parent.width
                    height: 7
                    border.style: "double"
                    border.color: "green"
                    color: "#0a140e"

                    Column {
                        id: cpuPanelColumn
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 0

                        Text {
                            text: "🖥️ CPU MULTI-CORE UTILIZATION (%)"
                            color: "green"
                            bold: true
                        }

                        // Core 0
                        Row {
                            spacing: 1
                            Text { text: " Core 0 (Kernel) : "; color: "green"; width: 19 }
                            Text {
                                text: (rootWindow.cpu0Val < 10 ? " " : "") + rootWindow.cpu0Val + "% "
                                color: rootWindow.cpu0Val > 80 ? "red" : "green"
                                width: 6
                            }
                            Sparkline {
                                width: Math.max(10, cpuPanelColumn.width - 28)
                                height: 1
                                values: rootWindow.cpu0History
                                minVal: 0
                                maxVal: 100
                                color: rootWindow.cpu0Val > 80 ? "red" : "green"
                            }
                        }

                        // Core 1
                        Row {
                            spacing: 1
                            Text { text: " Core 1 (Web)    : "; color: "cyan"; width: 19 }
                            Text {
                                text: (rootWindow.cpu1Val < 10 ? " " : "") + rootWindow.cpu1Val + "% "
                                color: rootWindow.cpu1Val > 80 ? "red" : "cyan"
                                width: 6
                            }
                            Sparkline {
                                width: Math.max(10, cpuPanelColumn.width - 28)
                                height: 1
                                values: rootWindow.cpu1History
                                minVal: 0
                                maxVal: 100
                                color: rootWindow.cpu1Val > 80 ? "red" : "cyan"
                            }
                        }

                        // Core 2
                        Row {
                            spacing: 1
                            Text { text: " Core 2 (DB)     : "; color: "yellow"; width: 19 }
                            Text {
                                text: (rootWindow.cpu2Val < 10 ? " " : "") + rootWindow.cpu2Val + "% "
                                color: rootWindow.cpu2Val > 80 ? "red" : "yellow"
                                width: 6
                            }
                            Sparkline {
                                width: Math.max(10, cpuPanelColumn.width - 28)
                                height: 1
                                values: rootWindow.cpu2History
                                minVal: 0
                                maxVal: 100
                                color: rootWindow.cpu2Val > 80 ? "red" : "yellow"
                            }
                        }

                        // Core 3
                        Row {
                            spacing: 1
                            Text { text: " Core 3 (Queue)  : "; color: "magenta"; width: 19 }
                            Text {
                                text: (rootWindow.cpu3Val < 10 ? " " : "") + rootWindow.cpu3Val + "% "
                                color: rootWindow.cpu3Val > 80 ? "red" : "magenta"
                                width: 6
                            }
                            Sparkline {
                                width: Math.max(10, cpuPanelColumn.width - 28)
                                height: 1
                                values: rootWindow.cpu3History
                                minVal: 0
                                maxVal: 100
                                color: rootWindow.cpu3Val > 80 ? "red" : "magenta"
                            }
                        }
                    }
                }

                // PANEL 2: MULTI-ROW VECTOR CURVE SPARKLINE
                Rectangle {
                    width: parent.width
                    height: 8
                    border.style: "double"
                    border.color: "yellow"
                    color: "#16140a"

                    Column {
                        id: tallPanelColumn
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 0

                        Row {
                            spacing: 2
                            Text {
                                text: "📊 MULTI-ROW VECTOR CURVE (Height: 4 rows)"
                                color: "yellow"
                                bold: true
                            }
                            Text {
                                text: "Avg Load: " + Math.round(rootWindow.avgCpu) + "%"
                                color: rootWindow.avgCpu > 70 ? "red" : "yellow"
                            }
                        }

                        Sparkline {
                            width: Math.max(10, tallPanelColumn.width - 2)
                            height: 4
                            filled: false
                            values: rootWindow.cpu1History
                            minVal: 0
                            maxVal: 100
                            color: rootWindow.avgCpu > 70 ? "red" : "yellow"
                        }
                    }
                }

                // PANEL 3: NETWORK & STORAGE I/O
                Rectangle {
                    width: parent.width
                    height: 6
                    border.style: "double"
                    border.color: "cyan"
                    color: "#0a131a"

                    Column {
                        id: netPanelColumn
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 0

                        Text {
                            text: "🌐 NETWORK BANDWIDTH & STORAGE I/O"
                            color: "cyan"
                            bold: true
                        }

                        // Net RX
                        Row {
                            spacing: 1
                            Text { text: " Network In (RX) : "; color: "cyan"; width: 19 }
                            Text {
                                text: (rootWindow.netRxVal < 10 ? " " : "") + rootWindow.netRxVal + " MB/s "
                                color: "cyan"
                                width: 12
                            }
                            Sparkline {
                                width: Math.max(10, netPanelColumn.width - 34)
                                height: 1
                                values: rootWindow.netRxHistory
                                minVal: 0
                                maxVal: 60
                                color: "cyan"
                            }
                        }

                        // Net TX
                        Row {
                            spacing: 1
                            Text { text: " Network Out (TX): "; color: "blue"; width: 19 }
                            Text {
                                text: (rootWindow.netTxVal < 10 ? " " : "") + rootWindow.netTxVal + " MB/s "
                                color: "blue"
                                width: 12
                            }
                            Sparkline {
                                width: Math.max(10, netPanelColumn.width - 34)
                                height: 1
                                values: rootWindow.netTxHistory
                                minVal: 0
                                maxVal: 60
                                color: "blue"
                            }
                        }

                        // Disk Read
                        Row {
                            spacing: 1
                            Text { text: " Disk Read IOPS  : "; color: "white"; width: 19 }
                            Text {
                                text: rootWindow.diskReadVal + " IOPS "
                                color: "white"
                                width: 12
                            }
                            Sparkline {
                                width: Math.max(10, netPanelColumn.width - 34)
                                height: 1
                                values: rootWindow.diskReadHistory
                                minVal: 0
                                maxVal: 1000
                                color: "white"
                            }
                        }
                    }
                }

                // PANEL 4: MEMORY & APP HEALTH
                Rectangle {
                    width: parent.width
                    height: 6
                    border.style: "double"
                    border.color: "magenta"
                    color: "#140a16"

                    Column {
                        id: memPanelColumn
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 0

                        Text {
                            text: "⚡ MEMORY ALLOCATION & SERVICE HEALTH"
                            color: "magenta"
                            bold: true
                        }

                        // RAM
                        Row {
                            spacing: 1
                            Text { text: " RAM Allocation  : "; color: "magenta"; width: 19 }
                            Text {
                                text: (rootWindow.memVal < 10 ? " " : "") + rootWindow.memVal + "% "
                                color: rootWindow.memVal > 80 ? "red" : "magenta"
                                width: 8
                            }
                            Sparkline {
                                width: Math.max(10, memPanelColumn.width - 30)
                                height: 1
                                values: rootWindow.memHistory
                                minVal: 0
                                maxVal: 100
                                color: rootWindow.memVal > 80 ? "red" : "magenta"
                            }
                        }

                        // Req Rate
                        Row {
                            spacing: 1
                            Text { text: " HTTP Req Rate   : "; color: "green"; width: 19 }
                            Text {
                                text: rootWindow.reqRateVal + " r/s "
                                color: "green"
                                width: 8
                            }
                            Sparkline {
                                width: Math.max(10, memPanelColumn.width - 30)
                                height: 1
                                values: rootWindow.reqRateHistory
                                minVal: 0
                                maxVal: 3500
                                color: "green"
                            }
                        }

                        // p99 Latency
                        Row {
                            spacing: 1
                            Text { text: " p99 API Latency : "; color: rootWindow.latencyVal > 80 ? "red" : "cyan"; width: 19 }
                            Text {
                                text: rootWindow.latencyVal + " ms "
                                color: rootWindow.latencyVal > 80 ? "red" : "cyan"
                                width: 8
                            }
                            Sparkline {
                                width: Math.max(10, memPanelColumn.width - 30)
                                height: 1
                                values: rootWindow.latencyHistory
                                minVal: 0
                                maxVal: 150
                                color: rootWindow.latencyVal > 80 ? "red" : "cyan"
                            }
                        }
                    }
                }
            }
        }
    }
}
