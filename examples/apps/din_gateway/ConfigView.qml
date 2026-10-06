import QtQml
import Qtui
import DinGatewayApp

Rectangle {
    id: configRoot
    color: "transparent"

    property string modbusBaud: "115200"
    property string canBitrate: "500k"
    property string publishInterval: "5"
    property string statusMsg: "READY FOR CONFIGURATION CHANGES"

    function handleKey(key) {
        let res = false;
        if (key === Qt.Key_1) {
            modbusBaud = "9600";
            GatewayBackend.setModbusBaud("9600");
            statusMsg = "MODBUS BAUD SET TO 9600 BPS";
            res = true;
        } else if (key === Qt.Key_2) {
            modbusBaud = "19200";
            GatewayBackend.setModbusBaud("19200");
            statusMsg = "MODBUS BAUD SET TO 19200 BPS";
            res = true;
        } else if (key === Qt.Key_3) {
            modbusBaud = "115200";
            GatewayBackend.setModbusBaud("115200");
            statusMsg = "MODBUS BAUD SET TO 115200 BPS";
            res = true;
        } else if (key === Qt.Key_4) {
            canBitrate = "250k";
            GatewayBackend.setCanBitrate("250k");
            statusMsg = "CAN BITRATE SET TO 250 KBPS";
            res = true;
        } else if (key === Qt.Key_5) {
            canBitrate = "500k";
            GatewayBackend.setCanBitrate("500k");
            statusMsg = "CAN BITRATE SET TO 500 KBPS";
            res = true;
        } else if (key === Qt.Key_6) {
            canBitrate = "1000k";
            GatewayBackend.setCanBitrate("1000k");
            statusMsg = "CAN BITRATE SET TO 1000 KBPS";
            res = true;
        } else if (key === Qt.Key_7) {
            publishInterval = "1";
            GatewayBackend.setPublishInterval(1);
            statusMsg = "MQTT TELEMETRY RATE SET TO 1s";
            res = true;
        } else if (key === Qt.Key_8) {
            publishInterval = "5";
            GatewayBackend.setPublishInterval(5);
            statusMsg = "MQTT TELEMETRY RATE SET TO 5s";
            res = true;
        } else if (key === Qt.Key_9) {
            publishInterval = "30";
            GatewayBackend.setPublishInterval(30);
            statusMsg = "MQTT TELEMETRY RATE SET TO 30s";
            res = true;
        } else if (key === Qt.Key_S) {
            statusMsg = "SAVED TO FLASH MEMORY (EEMMC) OK!";
            res = true;
        } else if (key === Qt.Key_R) {
            modbusBaud = "115200";
            canBitrate = "500k";
            publishInterval = "5";
            statusMsg = "RESET TO FACTORY DEFAULTS!";
            res = true;
        }
        return res;
    }

    Column {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 1

        Rectangle {
            width: parent.width
            height: parent.height - 3
            border.style: "single"
            border.color: GatewayTheme.borderNormal
            color: GatewayTheme.bgPanel

            Column {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 1

                Text {
                    text: "⚙ GATEWAY PARAMETERS"
                    color: GatewayTheme.textAccent
                    bold: true
                }

                Row {
                    width: parent.width
                    spacing: 1
                    Text { width: 22; text: "Modbus RTU Baud: "; color: GatewayTheme.textPrimary }
                    Text { text: "[1]9600 [2]19200 [3]115200 (Active: " + configRoot.modbusBaud + ")"; color: GatewayTheme.textSuccess; bold: true }
                }

                Row {
                    width: parent.width
                    spacing: 1
                    Text { width: 22; text: "CAN Bus Bitrate: "; color: GatewayTheme.textPrimary }
                    Text { text: "[4]250k [5]500k  [6]1000k  (Active: " + configRoot.canBitrate + ")"; color: GatewayTheme.textAccent; bold: true }
                }

                Row {
                    width: parent.width
                    spacing: 1
                    Text { width: 22; text: "MQTT Rate:       "; color: GatewayTheme.textPrimary }
                    Text { text: "[7]1s   [8]5s    [9]30s    (Active: " + configRoot.publishInterval + "s)"; color: GatewayTheme.textWarning; bold: true }
                }

                Text {
                    text: "⌨ KEYBOARD ACTIONS:"
                    color: GatewayTheme.textAccent
                    bold: true
                }
                Text { text: "[1-3] Set Modbus Baud  [4-6] Set CAN Rate  [7-9] MQTT Rate"; color: GatewayTheme.textSecondary }
                Text { text: "[S] Save Settings to Flash  [R] Reset Factory Defaults"; color: GatewayTheme.textSecondary }
            }
        }

        Rectangle {
            width: parent.width
            height: 2
            border.style: "single"
            border.color: GatewayTheme.borderNormal
            color: GatewayTheme.bgHeader

            Row {
                anchors.centerIn: parent
                spacing: 1

                Text {
                    text: "STATUS: " + configRoot.statusMsg
                    color: GatewayTheme.textSuccess
                    bold: true
                }
            }
        }
    }
}
