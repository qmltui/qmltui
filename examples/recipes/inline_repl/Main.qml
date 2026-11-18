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
                    color: "lime"
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
                    color: "lime"
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
                            bold: true
                        },
                        {
                            text: " "
                        },
                        {
                            text: " v0.6 ",
                            color: "black",
                            backgroundColor: "green",
                            bold: true
                        },
                        {
                            text: " · Inline Agent CLI",
                            color: "cyan"
                        }
                    ]
                }

                Text {
                    text: "✦ claude-sonnet-4 · ~/qml-tui · Terminal.Inline"
                    color: "gray"
                }

                Text {
                    text: "Retained-mode QML terminal UI with native scrollback history"
                    dim: true
                }

                Text {
                    text: "Type a command and press Enter to print into native scrollback"
                    dim: true
                }

                Text {
                    text: "Esc clears the prompt, then quits"
                    dim: true
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
                    color: "black",
                    backgroundColor: "cyan",
                    bold: true
                },
                {
                    text: "  UI sits in the normal buffer; printed lines scroll above it",
                    dim: true
                }
            ]
        }

        Row {
            spacing: 1

            Text {
                width: 8
                text: "load"
            }

            ProgressBar {
                width: 30
                from: 0
                to: 100
                value: root.load
                style: "segmented"
                color: "cyan"
                showPercentage: true
            }
        }

        Row {
            Text {
                text: "❯ "
                color: "cyan"
                bold: true
            }

            TextInput {
                id: prompt
                width: Math.max(20, root.width - 4)
                height: 1
                focus: true
                placeholderText: "type a line and press Enter to print into scrollback"
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
            dim: true
        }
    }
}
