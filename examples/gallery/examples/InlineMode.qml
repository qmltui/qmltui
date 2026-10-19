pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: inlineDemo
    property int printed: 0
    property string lastCommand: "ready"
    implicitHeight: 6

    Scrollback {
        id: scrollback
    }

    Panel {
        width: parent.width
        height: 6

        border {
            style: Border.Rounded
            color: "#30363d"
        }

        title: Row {
            spacing: 1
            Text {
                text: "Inline mode · Agent CLI"
                color: "#f8fafc"
            }
            Text {
                text: "[INLINE]"
                color: "#2cde85"
            }
        }
        titleTrailing: Text {
            text: "printed: " + inlineDemo.printed
            color: "#38bdf8"
        }
        footer: Text {
            text: "Enter: commit to Scrollback · Esc: exit"
            color: "#8b949e"
        }
        footerTrailing: Text {
            text: "last: " + inlineDemo.lastCommand
            color: "#2cde85"
        }
        Column {
            anchors {
                fill: parent
                margins: 1
            }
            spacing: 0

            Row {
                spacing: 2

                Text {
                    text: "Inline mode · printed: " + inlineDemo.printed
                    bold: true
                    color: "#2cde85"
                }

                ProgressBar {
                    width: 16
                    from: 0
                    to: 4
                    value: Math.min(4, inlineDemo.printed)
                    style: "segmented"
                    showBrackets: true
                    color: "#38bdf8"
                }
            }

            Row {
                width: parent.width
                spacing: 1

                Text {
                    text: "❯"
                    bold: true
                    color: "#38bdf8"
                }

                TextInput {
                    id: promptInput
                    width: Math.max(20, parent.width - 3)
                    height: 1
                    focus: true
                    text: ""
                    placeholderText: "Type a command and press Enter..."
                    placeholderColor: "#8b949e"
                    color: "#f8fafc"

                    onAccepted: {
                        const line = promptInput.text.length > 0 ? promptInput.text : "status --check";
                        scrollback.print("❯ " + line + "\n  ✓ committed #" + (inlineDemo.printed + 1));
                        inlineDemo.lastCommand = line;
                        inlineDemo.printed += 1;
                        promptInput.clear();
                    }
                }
            }

            Text {
                width: parent.width
                text: "Scrollback queue: 1 MiB · Committed rows persist in native history"
                color: "#8b949e"
            }
        }
    }
}
