pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Inline mode: Inline + Scrollback. One tile of the QmlTui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    mode: Terminal.Inline

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit();
    }

    Scrollback {
        id: history
    }

    Column {
        id: inlineDemo
        property int printed: 0
        property real load: 0
        visible: true
        width: parent.width

        NumberAnimation {
            target: inlineDemo
            property: "load"
            from: 0
            to: 100
            duration: 4000
            loops: -1
            running: true
        }

        Text {
            spans: [
                {
                    text: " Inline mode ",
                    color: "#0d1117",
                    backgroundColor: "#38bdf8",
                    bold: true
                },
                {
                    text: "  the UI sits at the bottom of the normal buffer; printed lines scroll above it",
                    color: "#8b949e"
                }
            ]
        }
        Row {
            spacing: 1
            Text {
                width: 8
                text: "load"
                color: "#c9d1d9"
            }
            ProgressBar {
                width: 30
                from: 0
                to: 100
                value: inlineDemo.load
                style: "segmented"
                color: "#38bdf8"
                showPercentage: true
            }
            Text {
                text: "  live value, animated while inline"
                color: "#8b949e"
            }
        }
        Row {
            Text {
                text: "❯ "
                color: "#38bdf8"
                bold: true
            }
            TextInput {
                id: prompt
                width: root.width - 2
                height: 1
                focus: true
                placeholderText: "type a line and press Enter to print it into the scrollback"
                color: "#c9d1d9"
                onAccepted: {
                    if (text.length === 0)
                        return;
                    if (history.print("❯ " + text)) {
                        inlineDemo.printed++;
                        text = "";
                    }
                }
            }
        }
        Text {
            text: "printed: " + inlineDemo.printed + "   Esc returns to the gallery (quits when run alone)"
            color: "#8b949e"
        }
    }
}
