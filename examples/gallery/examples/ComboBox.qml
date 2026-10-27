pragma ComponentBehavior: Bound
import QtQml.Models
import QmlTui
import QmlTui.Controls

// ComboBox: Default inline selector, role-bound ListModel (QAbstractItemModel),
// and custom-styled ComboBox using border, contentItem, and indicator slots.
Column {
    id: comboDemo

    anchors.centerIn: parent
    spacing: 1

    ListModel {
        id: regionModel
        ListElement {
            label: "eu-west-1 (Dublin)"
            code: "euw1"
        }
        ListElement {
            label: "us-east-1 (Virginia)"
            code: "use1"
        }
        ListElement {
            label: "ap-northeast-1 (Tokyo)"
            code: "apne1"
        }
        ListElement {
            label: "sa-east-1 (São Paulo)"
            code: "sae1"
        }
    }

    Text {
        text: "Enter/Space opens the list; Up/Down selects:"
        color: "#8b949e"
    }

    // Default inline ComboBox with string array model
    Row {
        spacing: 2

        Text {
            width: 14
            text: "Environment"
            color: "#c9d1d9"
            bold: true
        }

        ComboBox {
            id: envCombo
            width: 28
            focus: true
            model: ["Production", "Staging", "Development", "Local Sandbox"]
            textColor: "#2cde85"
            popupColor: "#0d1117"
            popupBorderColor: "#2cde85"
        }
    }

    // QAbstractItemModel / ListModel bound via textRole
    Row {
        spacing: 2

        Text {
            width: 14
            text: "Region (Role)"
            color: "#c9d1d9"
            bold: true
        }

        ComboBox {
            id: regionCombo
            width: 28
            model: regionModel
            textRole: "label"
            textColor: "#58a6ff"
            popupColor: "#0d1117"
            popupBorderColor: "#58a6ff"
        }
    }

    // Custom-styled ComboBox with Rounded/Double border, contentItem, and indicator
    Row {
        spacing: 2

        Text {
            y: 1
            width: 14
            text: "Custom Style"
            color: "#c9d1d9"
            bold: true
        }

        ComboBox {
            id: logCombo
            width: 28
            currentIndex: 1
            model: ["trace", "info", "warn", "error"]
            textColor: "#f59e0b"
            popupColor: "#0d1117"
            popupBorderColor: "#f59e0b"
            border {
                style: logCombo.activeFocus || logCombo.opened ? Border.Double : Border.Rounded
                color: logCombo.activeFocus || logCombo.opened ? "#fbbf24" : "#f59e0b"
            }
            contentItem: Text {
                text: "◆ " + logCombo.currentText
                clip: true
                color: "#f59e0b"
                bold: logCombo.activeFocus || logCombo.opened
            }
            indicator: Text {
                text: logCombo.opened ? "▲" : "▼"
                color: "#f59e0b"
                bold: true
            }
        }
    }

    Text {
        text: "Target: " + envCombo.currentText + "  ·  " + regionCombo.currentText + "  ·  log=" + logCombo.currentText
        color: "#2cde85"
    }
}
