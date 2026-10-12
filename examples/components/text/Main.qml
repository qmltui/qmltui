pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: rootWindow

    property bool alternateStyle: false
    property bool showSpans: true
    property real gradientOffset: 0.0

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Q) {
            Qt.quit();
        } else if (event.key === Qt.Key_S) {
            rootWindow.alternateStyle = !rootWindow.alternateStyle;
        } else if (event.key === Qt.Key_T) {
            rootWindow.showSpans = !rootWindow.showSpans;
        } else {
            return;
        }
        event.accepted = true;
    }

    Rectangle {
        anchors.fill: parent
        border.style: Border.Single
        border.color: "yellow"
    }

    NumberAnimation {
        target: rootWindow
        property: "gradientOffset"
        from: 0.0
        to: 1.0
        duration: 2000
        loops: -1
        running: true
    }

    Column {
        anchors.fill: parent
        anchors.margins: 2
        spacing: 1

        Text {
            text: "Text: styled spans, Unicode & gradients"
            color: "cyan"
            bold: true
        }

        Text {
            text: "Emojis:  😀 😎 🚀 🎉 🎯 🍕\nFlags:   🇺🇸 🇫🇷 🇩🇪 🇯🇵 🇨🇦 🇧🇷\nHands:   1️⃣ 2️⃣ 👍 👍🏽 👏"
        }

        // Object literals and the factory produce the same span value type.
        Text {
            spans: [
                {
                    text: "Inline: ",
                    dim: true
                },
                QmlTui.span(" READY ", {
                    color: "#001b10",
                    backgroundColor: "#65e6a0",
                    bold: true
                }),
                {
                    text: "  warning",
                    color: "yellow",
                    underline: true
                },
                {
                    text: "  error",
                    color: "red",
                    reversed: true
                }
            ]
        }

        // A newline starts a line; adjacent spans continue on that line.
        Text {
            spans: [
                {
                    text: "Status: ",
                    dim: true
                },
                QmlTui.span("Active\n", {
                    color: "green",
                    bold: true
                }),
                {
                    text: "Workers: ",
                    dim: true
                },
                {
                    text: "4",
                    bold: true
                }
            ]
        }

        Text {
            color: rootWindow.alternateStyle ? "#ffb86c" : "#8be9fd"
            bold: rootWindow.alternateStyle
            text: "Fallback text: press T to restore spans"
            spans: rootWindow.showSpans ? [
                {
                    text: "Inherited"
                },
                {
                    text: " | explicit green",
                    color: "green",
                    bold: false
                },
                {
                    text: " | terminal default",
                    color: "default",
                    bold: false
                }
            ] : []
        }

        Text {
            width: parent.width
            bold: true
            text: "MOVING COLOR GRADIENT TEXT DEMO"
            gradient: Gradient {
                offset: rootWindow.gradientOffset
                GradientStop {
                    position: 0.0
                    color: "magenta"
                }
                GradientStop {
                    position: 0.33
                    color: "cyan"
                }
                GradientStop {
                    position: 0.66
                    color: "yellow"
                }
                GradientStop {
                    position: 1.0
                    color: "magenta"
                }
            }
        }

        Text {
            color: "orange"
            text: "  /\\_/\\\n ( o.o )   Auto-sized ASCII cat\n  > ^ <    Three explicit lines"
        }

        Text {
            text: "S: change base style  T: toggle spans  Q / Esc: quit"
            dim: true
        }
    }
}
