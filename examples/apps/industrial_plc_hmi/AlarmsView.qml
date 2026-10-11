pragma ComponentBehavior: Bound
import QtQml
import Qamui
import IndustrialPlcHmi

Rectangle {
    id: root
    property var eventModel: null
    property int errorCount: 0

    anchors.fill: parent
    color: HmiTheme.cardBg
    border.style: "single"
    border.color: HmiTheme.borderNormal

    Column {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

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

        Column {
            id: logRows
            width: parent.width
            height: Math.max(0, parent.height - 3)
            spacing: 0

            Repeater {
                model: root.eventModel

                delegate: Rectangle {
                    id: eventRow
                    required property int index
                    required property int sequence
                    required property string severity
                    required property string message

                    width: logRows.width
                    height: 1
                    color: index % 2 === 0 ? "#181a1f" : "#1e222b"

                    Row {
                        spacing: 2
                        Text {
                            text: " #" + String(eventRow.sequence).padStart(2, "0") + " "
                            color: HmiTheme.textDim
                        }
                        Text {
                            width: 10
                            text: "[" + eventRow.severity + "]"
                            color: root.severityColor(eventRow.severity)
                            bold: eventRow.severity === "CRITICAL"
                        }
                        Text {
                            text: eventRow.message
                            color: HmiTheme.textPrimary
                        }
                    }
                }
            }
        }

        Text {
            text: "Press [R] to clear fault lockout when in ALARM state"
            color: HmiTheme.textWarning
        }
    }

    function severityColor(severity) {
        if (severity === "CRITICAL")
            return HmiTheme.textDanger
        if (severity === "WARNING")
            return HmiTheme.textWarning
        return HmiTheme.textSuccess
    }
}
