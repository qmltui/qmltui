pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// Clipboard: Copies text to the system clipboard via OSC 52 and emits
// copyRequested(text) so the application can display copy confirmation.
Column {
    id: clipDemo

    property string lastCopied: "(nothing copied yet)"
    property int copyCount: 0

    anchors.centerIn: parent
    spacing: 1

    Connections {
        target: Clipboard
        function onCopyRequested(text: string) {
            clipDemo.lastCopied = text;
            clipDemo.copyCount += 1;
        }
    }

    Text {
        text: "Click a button or press Enter to copy text via Clipboard.copy():"
        color: "#8b949e"
    }

    Row {
        spacing: 2

        Rectangle {
            width: 34
            height: 3
            color: customField.activeFocus ? "#122d24" : "#161b22"

            TextInput {
                id: customField
                x: 2
                y: 1
                width: 30
                height: 1
                text: "ssh deploy@prod.example.com"
                color: "#c9d1d9"
            }
        }

        Button {
            text: "Copy Input"
            focus: true
            textColor: "#2cde85"
            onClicked: Clipboard.copy(customField.text)
        }
    }

    Row {
        spacing: 2

        Button {
            text: "Copy Build Cmd"
            textColor: "#58a6ff"
            onClicked: Clipboard.copy("cmake --build build -j8")
        }

        Button {
            text: "Copy Token"
            textColor: "#f59e0b"
            onClicked: Clipboard.copy("qtui_live_9f8a7b6c5d4e")
        }
    }

    Rectangle {
        width: 52
        height: 4
        color: "#161b22"

        Column {
            x: 2
            y: 0
            spacing: 0

            Row {
                spacing: 2

                Text {
                    text: "Clipboard Event Log"
                    color: "#2cde85"
                    bold: true
                }

                Text {
                    text: "(" + clipDemo.copyCount + (clipDemo.copyCount === 1 ? " copy)" : " copies)")
                    color: "#8b949e"
                }
            }

            Text {
                text: "Last payload: " + clipDemo.lastCopied
                color: clipDemo.copyCount > 0 ? "#e6edf3" : "#8b949e"
                bold: clipDemo.copyCount > 0
            }

            Text {
                text: "Length: " + (clipDemo.copyCount > 0 ? clipDemo.lastCopied.length + " chars (OSC 52)" : "0 chars")
                color: "#58a6ff"
            }
        }
    }
}
