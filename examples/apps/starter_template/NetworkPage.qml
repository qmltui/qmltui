import QtQml
import Qtui
import Qtui.Controls
import StarterTemplateApp

Rectangle {
    id: page

    color: Theme.bgMain

    // Root Terminal item, used for OSC52 clipboard copy of the deploy command.
    property var terminal: null
    readonly property string deployCommand: WifiManager.deployIp.length > 0 ? "ssh root@" + WifiManager.deployIp : ""

    property int selectedIndex: 0
    property string selectedSsid: ""
    property string selectedSecurity: ""
    property int selectedSignal: 0
    property bool isConnecting: false
    property string validationError: ""

    focusable: true

    state: WifiManager.connected ? "connected" : "disconnected"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_S || event.key === Qt.Key_R) {
            if (!WifiManager.connected) {
                WifiManager.scan();
                event.accepted = true;
            }
        } else if (event.key === Qt.Key_Up) {
            if (page.selectedIndex > 0) {
                page.selectedIndex -= 1;
                page.updateSelection();
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Down) {
            if (page.selectedIndex < WifiManager.networks.length - 1) {
                page.selectedIndex += 1;
                page.updateSelection();
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            if (!WifiManager.connected) {
                if (pwdInput.activeFocus) {
                    page.doConnect();
                } else {
                    pwdInput.focus = true;
                }
            }
            event.accepted = true;
        }
    }

    function updateSelection() {
        if (WifiManager.networks.length > page.selectedIndex) {
            var net = WifiManager.networks[page.selectedIndex];
            page.selectedSsid = net.ssid;
            page.selectedSecurity = net.security;
            page.selectedSignal = net.signal;
            page.validationError = "";

            // Auto-scroll to keep active item in viewport (height: 3 + spacing: 1 = 4)
            var itemTop = page.selectedIndex * 4;
            var itemBottom = itemTop + 3;
            var viewHeight = networkScrollView.height;
            if (itemTop < networkScrollView.contentY) {
                networkScrollView.contentY = itemTop;
            } else if (itemBottom > networkScrollView.contentY + viewHeight) {
                networkScrollView.contentY = Math.max(0, itemBottom - viewHeight);
            }
        }
    }

    function doConnect() {
        if (page.selectedSsid.length === 0)
            return;

        // Validate password length for secured networks
        if (page.selectedSecurity !== "Open" && pwdInput.text.length < 8) {
            page.validationError = "Passphrase must be at least 8 characters";
            return;
        }

        page.validationError = "";
        page.isConnecting = true;
        WifiManager.connectToNetwork(page.selectedSsid, pwdInput.text);
    }

    Component.onCompleted: {
        page.updateSelection();
    }

    Connections {
        target: WifiManager
        function onNetworksChanged() {
            page.updateSelection();
        }
        function onConnectionResult(success, message) {
            page.isConnecting = false;
        }
    }

    // =========================================================================
    // STATE 1: DISCONNECTED VIEW (Scanner List + Connect Form)
    // =========================================================================
    Rectangle {
        id: disconnectedView
        anchors.fill: parent
        color: Theme.bgMain
        visible: page.state === "disconnected"

        // Left: Wireless network scanner
        Panel {
            id: listPanel
            anchors.top: parent.top
            anchors.bottom: logPanel.top
            anchors.left: parent.left
            anchors.margins: 1
            anchors.bottomMargin: 0
            width: Math.min(84, Math.floor((parent.width - 3) / 2))
            color: Theme.bgPanel
            border.color: Theme.borderNormal
            title: WifiManager.scanning ? Theme.iconScan + "Scanning RF Channels..." : Theme.iconWifi + "Wireless Networks (" + WifiManager.networks.length + ")"
            titleColor: Theme.qtGreen

            ScrollView {
                id: networkScrollView
                anchors.top: parent.top
                anchors.bottom: scanBtnRow.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 1
                anchors.topMargin: 2
                scrollBarVisible: false
                clip: true

                Column {
                    width: networkScrollView.width
                    spacing: 1

                    Repeater {
                        model: WifiManager.networks

                        Rectangle {
                            width: networkScrollView.width
                            height: 3
                            color: index === page.selectedIndex ? Theme.bgActive : Theme.bgPanel
                            border.style: "single"
                            border.color: index === page.selectedIndex ? Theme.qtGreen : Theme.borderNormal

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    page.selectedIndex = index;
                                    page.updateSelection();
                                    pwdInput.focus = true;
                                }
                            }

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 2
                                spacing: 2

                                Text {
                                    text: index === page.selectedIndex ? Theme.pointer : " "
                                    color: Theme.qtGreen
                                    bold: true
                                }

                                SignalMeter {
                                    strength: modelData.signal
                                }

                                Text {
                                    text: modelData.ssid
                                    color: Theme.textPrimary
                                    bold: (index === page.selectedIndex)
                                }

                                StatusBadge {
                                    text: modelData.security
                                    badgeColor: modelData.security === "Open" ? Theme.textSecondary : Theme.qtGreen
                                    showIcon: false
                                }

                                Text {
                                    text: modelData.connected ? Theme.check + " CONNECTED" : ""
                                    color: Theme.qtGreen
                                    bold: true
                                }
                            }
                        }
                    }
                }
            }

            Text {
                visible: WifiManager.networks.length === 0
                anchors.centerIn: parent
                text: WifiManager.scanning ? "Scanning for access points..." : "No wireless networks found. Press [R] to rescan."
                color: Theme.textDim
            }

            // Scanner action bar
            Row {
                id: scanBtnRow
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 1
                anchors.leftMargin: 2
                spacing: 2

                Button {
                    label: WifiManager.scanning ? "Scanning..." : "[R] Rescan Now"
                    textColor: Theme.textPrimary
                    normalBorderColor: Theme.borderNormal
                    focusBorderColor: Theme.qtGreen
                    enabled: !WifiManager.scanning
                    onClicked: {
                        WifiManager.scan();
                    }
                }

                Button {
                    label: "[Enter] Connect"
                    textColor: Theme.qtGreen
                    normalBorderColor: Theme.qtGreen
                    focusBorderColor: Theme.qtGreen
                    onClicked: {
                        pwdInput.focus = true;
                    }
                }
            }
        }

        // Right Top: Wi-Fi credentials
        Panel {
            id: authPanel
            anchors.top: parent.top
            anchors.left: listPanel.right
            anchors.right: parent.right
            anchors.margins: 1
            height: 12
            color: Theme.bgPanel
            border.color: Theme.borderNormal
            title: Theme.iconAuth + "Wi-Fi Authentication"
            titleColor: Theme.qtGreen

            Column {
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                anchors.rightMargin: 2
                spacing: 1

                Row {
                    spacing: 2
                    Text {
                        text: "Target SSID: "
                        color: Theme.textSecondary
                    }
                    Text {
                        text: page.selectedSsid ? page.selectedSsid : "None Selected"
                        color: Theme.qtGreen
                        bold: true
                    }
                    Text {
                        text: "(" + (page.selectedSecurity ? page.selectedSecurity : "Open") + ")"
                        color: Theme.textDim
                    }
                    SignalMeter {
                        strength: page.selectedSignal
                    }
                }

                Rectangle {
                    width: Math.min(48, parent.width)
                    height: 3
                    color: Theme.bgMain
                    border.style: "single"
                    border.color: pwdInput.activeFocus ? Theme.qtGreen : (page.validationError.length > 0 ? Theme.statusError : Theme.borderNormal)

                    TextInput {
                        id: pwdInput
                        anchors.fill: parent
                        anchors.margins: 1
                        placeholderText: "Enter WPA2/WPA3 password..."
                        echoMode: TextInput.Password
                        color: Theme.textPrimary
                        onAccepted: {
                            page.doConnect();
                        }
                    }
                }

                // Validation / Error Banner
                Text {
                    visible: page.validationError.length > 0 || WifiManager.lastError.length > 0
                    text: page.validationError.length > 0 ? "[!] " + page.validationError : "[!] " + WifiManager.lastError
                    color: Theme.statusError
                    bold: true
                }

                Row {
                    spacing: 2

                    Button {
                        label: page.isConnecting ? "Connecting..." : Theme.check + " Connect [Enter]"
                        textColor: Theme.qtGreen
                        normalBorderColor: Theme.qtGreen
                        focusBorderColor: Theme.qtGreen
                        enabled: !page.isConnecting && page.selectedSsid.length > 0
                        onClicked: {
                            page.doConnect();
                        }
                    }
                }
            }
        }

        // Right Middle: SSH access
        Panel {
            id: sshPanel
            anchors.top: authPanel.bottom
            anchors.left: listPanel.right
            anchors.right: parent.right
            anchors.margins: 1
            anchors.topMargin: 0
            height: 3
            color: Theme.bgPanel
            border.color: Theme.borderNormal
            title: Theme.iconKey + "Remote Access"
            titleColor: Theme.qtGreen

            property bool copied: false

            Timer {
                id: copiedResetTimer
                interval: 2000
                onTriggered: sshPanel.copied = false
            }

            Row {
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                anchors.rightMargin: 2
                spacing: 2

                Text {
                    text: "SSH:"
                    color: Theme.textSecondary
                }

                Text {
                    text: WifiManager.sshRunning ? Theme.dotOn + " " + WifiManager.sshServerName : Theme.dotOff + " off"
                    color: WifiManager.sshRunning ? Theme.qtGreen : Theme.statusWarn
                    bold: WifiManager.sshRunning
                }

                Text {
                    text: page.deployCommand.length > 0 ? page.deployCommand : "no address yet"
                    color: page.deployCommand.length > 0 ? Theme.qtGreen : Theme.textDim
                    bold: page.deployCommand.length > 0
                }

                Text {
                    visible: page.deployCommand.length > 0 && page.terminal !== null
                    text: sshPanel.copied ? Theme.check + " Copied" : "[ Copy ]"
                    color: sshPanel.copied ? Theme.qtGreen : Theme.textSecondary

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            page.terminal.copyToClipboard(page.deployCommand);
                            sshPanel.copied = true;
                            copiedResetTimer.restart();
                        }
                    }
                }
            }
        }

        // Right Bottom: Network Interfaces Panel
        Panel {
            id: ifacePanel
            anchors.top: sshPanel.bottom
            anchors.bottom: parent.bottom
            anchors.left: listPanel.right
            anchors.right: parent.right
            anchors.margins: 1
            anchors.topMargin: 0
            color: Theme.bgPanel
            border.color: Theme.borderNormal
            title: Theme.iconGlobe + "Network Interfaces & Assigned IPs"
            titleColor: Theme.qtGreen

            Column {
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                anchors.rightMargin: 2

                Repeater {
                    model: WifiManager.allInterfaces

                    Column {
                        Row {
                            spacing: 2

                            Text {
                                visible: !Theme.simpleGlyphs
                                text: modelData.type === "wifi" ? "📶" : "🌐"
                            }

                            Text {
                                text: modelData.displayName
                                color: Theme.textPrimary
                                bold: true
                            }

                            StatusBadge {
                                text: modelData.connected ? "ONLINE" : (modelData.hasLink ? "CARRIER" : "NO LINK")
                                badgeColor: modelData.connected ? Theme.statusOk : (modelData.hasLink ? Theme.statusWarn : Theme.textDim)
                                showIcon: false
                            }
                        }

                        Text {
                            readonly property string addresses: "   "
                                + (modelData.ip !== "-" ? "v4 " + modelData.ip + "  " : "")
                                + (modelData.ip6 !== "-" ? "v6 " + modelData.ip6 : "")
                            readonly property int maxChars: Math.max(0, ifacePanel.width - 6)
                            visible: modelData.ip !== "-" || modelData.ip6 !== "-"
                            text: addresses.length > maxChars ? addresses.substring(0, maxChars) : addresses
                            color: modelData.connected ? Theme.qtGreen : Theme.textSecondary
                        }
                    }
                }
            }
        }

        // Left Bottom: Event Log Panel
        Panel {
            id: logPanel
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: 1
            width: listPanel.width
            height: 7
            color: Theme.bgPanel
            border.color: Theme.borderNormal
            title: Theme.iconLog + "System & Connection Logs"
            titleColor: Theme.textSecondary

            Column {
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                anchors.rightMargin: 2
                spacing: 1

                Repeater {
                    model: WifiManager.logs.slice(0, Math.max(0, Math.floor((logPanel.height - 2) / 2)))

                    Text {
                        readonly property int maxChars: Math.max(0, logPanel.width - 6)
                        text: modelData.length > maxChars ? modelData.substring(0, maxChars) : modelData
                        color: modelData.indexOf("SUCCESS") >= 0 ? Theme.statusOk : (modelData.indexOf("ERROR") >= 0 || modelData.indexOf("WARNING") >= 0 ? Theme.statusError : Theme.textSecondary)
                    }
                }
            }
        }
    }

    // =========================================================================
    // STATE 2: CONNECTED VIEW (Active Connection Card + Halts Discovery)
    // =========================================================================
    Rectangle {
        id: connectedView
        anchors.fill: parent
        color: Theme.bgMain
        visible: page.state === "connected"

        // Top: Active Connection Card
        Panel {
            id: connectedCard
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 1
            height: 11
            color: Theme.bgPanel
            border.color: Theme.qtGreen
            title: Theme.iconWifi + "Active Wireless Connection"
            titleColor: Theme.qtGreen

            property bool copied: false

            Timer {
                id: connectedCopiedResetTimer
                interval: 2000
                onTriggered: connectedCard.copied = false
            }

            Column {
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                anchors.rightMargin: 2
                spacing: 1

                Row {
                    spacing: 3

                    Text {
                        text: "Connected SSID: "
                        color: Theme.textSecondary
                    }

                    Text {
                        text: WifiManager.currentSsid
                        color: Theme.qtGreen
                        bold: true
                    }

                    StatusBadge {
                        text: "ONLINE"
                        badgeColor: Theme.statusOk
                        showIcon: false
                    }

                    SignalMeter {
                        strength: WifiManager.currentSignal
                    }

                    Text {
                        text: "IPv4: " + WifiManager.ipAddress
                        color: Theme.textPrimary
                        bold: true
                    }
                }

                Row {
                    spacing: 3

                    Text {
                        text: "Interface: " + WifiManager.interfaceName
                        color: Theme.textDim
                    }

                    Text {
                        text: "Status: Auto-Discovery Paused | Link Active"
                        color: Theme.textSecondary
                    }
                }

                Row {
                    spacing: 2

                    Text {
                        text: "SSH:"
                        color: Theme.textSecondary
                    }

                    Text {
                        text: WifiManager.sshRunning ? Theme.dotOn + " " + WifiManager.sshServerName : Theme.dotOff + " off"
                        color: WifiManager.sshRunning ? Theme.qtGreen : Theme.statusWarn
                        bold: WifiManager.sshRunning
                    }

                    Text {
                        text: page.deployCommand.length > 0 ? page.deployCommand : "no address yet"
                        color: page.deployCommand.length > 0 ? Theme.qtGreen : Theme.textDim
                        bold: page.deployCommand.length > 0
                    }

                    Text {
                        visible: page.deployCommand.length > 0 && page.terminal !== null
                        text: connectedCard.copied ? Theme.check + " Copied" : "[ Copy ]"
                        color: connectedCard.copied ? Theme.qtGreen : Theme.textSecondary

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                page.terminal.copyToClipboard(page.deployCommand);
                                connectedCard.copied = true;
                                connectedCopiedResetTimer.restart();
                            }
                        }
                    }
                }

                Row {
                    spacing: 2

                    Button {
                        label: Theme.cross + " Disconnect & Switch Network"
                        textColor: Theme.statusError
                        normalBorderColor: Theme.statusError
                        focusBorderColor: Theme.statusError
                        onClicked: {
                            WifiManager.disconnectNetwork();
                        }
                    }
                }
            }
        }

        // Bottom Left: Network Interfaces Panel
        Panel {
            id: connectedIfacePanel
            anchors.top: connectedCard.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: 1
            anchors.topMargin: 0
            width: Math.floor((parent.width - 3) / 2)
            color: Theme.bgPanel
            border.color: Theme.borderNormal
            title: Theme.iconGlobe + "Network Interfaces & Assigned IPs"
            titleColor: Theme.qtGreen

            Column {
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                anchors.rightMargin: 2

                Repeater {
                    model: WifiManager.allInterfaces

                    Column {
                        Row {
                            spacing: 2

                            Text {
                                visible: !Theme.simpleGlyphs
                                text: modelData.type === "wifi" ? "📶" : "🌐"
                            }

                            Text {
                                text: modelData.displayName
                                color: Theme.textPrimary
                                bold: true
                            }

                            StatusBadge {
                                text: modelData.connected ? "ONLINE" : (modelData.hasLink ? "CARRIER" : "NO LINK")
                                badgeColor: modelData.connected ? Theme.statusOk : (modelData.hasLink ? Theme.statusWarn : Theme.textDim)
                                showIcon: false
                            }
                        }

                        Text {
                            readonly property string addresses: "   "
                                + (modelData.ip !== "-" ? "v4 " + modelData.ip + "  " : "")
                                + (modelData.ip6 !== "-" ? "v6 " + modelData.ip6 : "")
                            readonly property int maxChars: Math.max(0, connectedIfacePanel.width - 6)
                            visible: modelData.ip !== "-" || modelData.ip6 !== "-"
                            text: addresses.length > maxChars ? addresses.substring(0, maxChars) : addresses
                            color: modelData.connected ? Theme.qtGreen : Theme.textSecondary
                        }
                    }
                }
            }
        }

        // Bottom Right: System & Connection Logs
        Panel {
            id: connectedLogPanel
            anchors.top: connectedCard.bottom
            anchors.bottom: parent.bottom
            anchors.left: connectedIfacePanel.right
            anchors.right: parent.right
            anchors.margins: 1
            anchors.topMargin: 0
            color: Theme.bgPanel
            border.color: Theme.borderNormal
            title: Theme.iconLog + "System & Connection Logs"
            titleColor: Theme.textSecondary

            Column {
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                anchors.rightMargin: 2
                spacing: 1

                Repeater {
                    model: WifiManager.logs.slice(0, Math.max(0, Math.floor((connectedLogPanel.height - 2) / 2)))

                    Text {
                        readonly property int maxChars: Math.max(0, connectedLogPanel.width - 6)
                        text: modelData.length > maxChars ? modelData.substring(0, maxChars) : modelData
                        color: modelData.indexOf("SUCCESS") >= 0 ? Theme.statusOk : (modelData.indexOf("ERROR") >= 0 || modelData.indexOf("WARNING") >= 0 ? Theme.statusError : Theme.textSecondary)
                    }
                }
            }
        }
    }
}
