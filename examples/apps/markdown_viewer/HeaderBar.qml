import QtQml
import Qtui

Rectangle {
    id: header
    height: 3
    color: theme.surface

    required property var theme
    property string title: "Document"
    property int currentLine: 1
    property int totalLines: 1
    property int headingCount: 0
    property int codeBlockCount: 0
    property int tableCount: 0
    property string activeThemeName: "aurora"
    property string activeStyleName: "modern"

    border.style: "single"
    border.color: theme.border

    Row {
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        spacing: 2

        Text {
            property int maxTitleLen: Math.max(10, header.width - 55)
            text: "📖 " + (header.title.length > maxTitleLen ? header.title.substring(0, maxTitleLen - 1) + "…" : header.title)
            color: theme.accent
            bold: true
        }

        Text {
            text: "│"
            color: theme.border
        }

        Text {
            text: "[H:" + header.headingCount + " C:" + header.codeBlockCount + " T:" + header.tableCount + "]"
            color: theme.textMuted
        }

        Text {
            text: "│"
            color: theme.border
        }

        Text {
            property int pct: (header.totalLines > 0) ? Math.min(100, Math.round((header.currentLine / header.totalLines) * 100)) : 0
            text: "Ln " + header.currentLine + "/" + header.totalLines + " (" + pct + "%)"
            color: theme.warning
            bold: true
        }

        Text {
            text: "│"
            color: theme.border
        }

        Text {
            text: "[" + header.activeThemeName + " : " + header.activeStyleName + "]"
            color: theme.accentSecondary
        }
    }
}
