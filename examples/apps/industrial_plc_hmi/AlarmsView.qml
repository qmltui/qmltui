import QtQml 2.15
import Qtui
import IndustrialPlcHmi 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: HmiTheme.cardBg
    border.style: "single"
    border.color: HmiTheme.borderNormal

    property var logs: []
    property int errorCount: 0

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Rectangle {
            width: parent.width
            height: 1
            color: "#282c34"
            Text {
                x: 1
                text: "🚨 ALARM HISTORY & AUDIT LOG (" + root.errorCount + " FAULTS RECORDED)"
                color: root.errorCount > 0 ? HmiTheme.textDanger : HmiTheme.textHighlight
                bold: true
            }
        }

        // Table Header
        Rectangle {
            width: parent.width
            height: 1
            color: "#21252b"
            Row {
                spacing: 2
                Text { text: " ID "; color: HmiTheme.textDim; bold: true }
                Text { text: "SEVERITY "; color: HmiTheme.textDim; bold: true }
                Text { text: "EVENT MESSAGE / SAFETY TRIGGER"; color: HmiTheme.textDim; bold: true }
            }
        }

        // Log Rows
        Repeater {
            model: Math.min(10, root.logs.length)
            delegate: Rectangle {
                width: parent.width
                height: 1
                color: index % 2 === 0 ? "#181a1f" : "#1e222b"

                Row {
                    spacing: 2
                    Text { 
                        text: " #" + (index + 1).toString().padStart(2, '0') + " "
                        color: HmiTheme.textDim 
                    }
                    Text { 
                        text: root.logs[index].indexOf("ALARM") >= 0 ? "[CRITICAL]" : "[INFO]    "
                        color: root.logs[index].indexOf("ALARM") >= 0 ? HmiTheme.textDanger : HmiTheme.textSuccess
                        bold: root.logs[index].indexOf("ALARM") >= 0
                    }
                    Text { 
                        text: root.logs[index]
                        color: HmiTheme.textPrimary 
                    }
                }
            }
        }

        Text { text: ""; height: 1 }

        Text {
            text: "Press [R] / [F3] to Clear Fault Lockout when in ALARM state"
            color: HmiTheme.textWarning
        }
    }
}
