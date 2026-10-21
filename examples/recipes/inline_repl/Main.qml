pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// Inline REPL: Renders at the bottom of the normal terminal buffer while
// Scrollback commits a startup agent banner and printed lines above the prompt.
Terminal {
    id: root

    property int printed: 0
    property real load: 0

    mode: Terminal.Inline
    title: "Inline REPL"

    Component.onCompleted: history.append({})

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            if (prompt.text.length > 0)
                prompt.text = "";
            else
                Qt.quit();
            event.accepted = true;
        }
    }

    Scrollback {
        id: history

        Row {
            height: 6
            spacing: 2

            Canvas {
                width: 11
                height: 5
                renderMode: Canvas.HalfBlock
                color: "#f8fafc"

                // Center the nine-pixel-wide QmlTui mark (ring + chevron tail).
                Circle {
                    cx: 5
                    cy: 4
                    radius: 3
                    fill: false
                }

                Circle {
                    cx: 5
                    cy: 4
                    radius: 4
                    fill: false
                }

                Polygon {
                    closed: false
                    color: "#00ff66"
                    points: [
                        {
                            x: 6,
                            y: 5
                        },
                        {
                            x: 8,
                            y: 7
                        },
                        {
                            x: 6,
                            y: 9
                        }
                    ]
                }

                Polygon {
                    closed: false
                    color: "#00ff66"
                    points: [
                        {
                            x: 7,
                            y: 5
                        },
                        {
                            x: 9,
                            y: 7
                        },
                        {
                            x: 7,
                            y: 9
                        }
                    ]
                }
            }

            Column {
                spacing: 0

                Text {
                    spans: [
                        {
                            text: "QmlTui",
                            color: "#f8fafc",
                            bold: true
                        },
                        {
                            text: " ",
                            color: "#8b949e"
                        },
                        {
                            text: " v0.5 ",
                            color: "#0d1117",
                            backgroundColor: "#2cde85",
                            bold: true
                        },
                        {
                            text: " · Inline Agent CLI",
                            color: "#38bdf8"
                        }
                    ]
                }

                Text {
                    text: "✦ claude-sonnet-4 · ~/qml-tui · Terminal.Inline"
                    color: "#94a3b8"
                }

                Text {
                    text: "Retained-mode QML terminal UI with native scrollback history"
                    color: "#8b949e"
                }

                Text {
                    text: "Type a command and press Enter to print into native scrollback"
                    color: "#64748b"
                }

                Text {
                    text: "Esc clears the prompt, then quits"
                    color: "#64748b"
                }
            }
        }
    }

    NumberAnimation {
        target: root
        property: "load"
        from: 0
        to: 100
        duration: 4000
        loops: -1
        running: true
    }

    Column {
        width: parent.width
        spacing: 1

        Text {
            spans: [
                {
                    text: " Inline mode ",
                    color: "#0d1117",
                    backgroundColor: "#38bdf8",
                    bold: true
                },
                {
                    text: "  UI sits in the normal buffer; printed lines scroll above it",
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
                value: root.load
                style: "segmented"
                color: "#38bdf8"
                showPercentage: true
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
                width: Math.max(20, root.width - 4)
                height: 1
                focus: true
                placeholderText: "type a line and press Enter to print into scrollback"
                color: "#c9d1d9"
                onAccepted: {
                    if (text.length === 0)
                        return;
                    if (history.print({
                        text: "❯ " + text,
                        color: "cyan"
                    })) {
                        root.printed++;
                        text = "";
                    }
                }
            }
        }

        Text {
            text: "printed: " + root.printed + "   Enter a message. Escape clears the prompt, then quits."
            color: "#8b949e"
        }
    }
}
