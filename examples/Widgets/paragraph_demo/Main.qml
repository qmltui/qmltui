import QtQml 2.15
import Terminal 1.0
import Terminal.Widgets 1.0

Terminal {
    width: 72
    height: 18

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        spacing: 1

        Text {
            text: "📄 PARAGRAPH HYPERTEXT LINKS DEMO"
            color: "cyan"
            bold: true
        }

        Text {
            text: "Click on any link to trigger linkActivated signal! Q/ESC to quit"
            color: "gray"
        }

        Paragraph {
            width: 70
            height: 10
            color: "white"
            borderStyle: "single"
            borderColor: "cyan"
            wrapMode: true
            textFormat: Paragraph.RichText
            text: "<b>Qtui Framework</b>: Build <font color='cyan'><b>high-performance</b></font> terminal applications powered by <b>Qt 6</b>.\n\nUseful Links:\n• <a href='https://qt.io'>Qt Official Documentation</a>\n• [Qtui GitHub Repository](https://github.com/yosriayed/qtui)\n• <a href='https://doc.qt.io/qt-6/'>Qt 6 QML Reference Manual</a>\n\nClick any link to activate!"

            onLinkActivated: (url) => {
                statusText.text = "Activated Hyperlink: " + url
            }
        }

        Text {
            id: statusText
            text: "Status: Click any link above..."
            color: "yellow"
            bold: true
        }
    }
}
