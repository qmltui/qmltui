pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// Color: SGR default/named colors, indexed ANSI palette (QmlTui.ansi), and 24-bit RGB.
Column {
    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "1. SGR Default & Named Colors (border-only Rectangles)"
        color: "#8b949e"
    }

    Row {
        spacing: 1

        Rectangle {
            width: 12
            height: 3
            border {
                style: Border.Rounded
                color: QmlTui.defaultColor
            }

            Text {
                anchors.centerIn: parent
                text: "default"
                color: QmlTui.defaultColor
            }
        }

        Rectangle {
            width: 12
            height: 3
            border {
                style: Border.Rounded
                color: "cyan"
            }

            Text {
                anchors.centerIn: parent
                text: "cyan"
                color: "cyan"
            }
        }

        Rectangle {
            width: 12
            height: 3
            border {
                style: Border.Rounded
                color: "green"
            }

            Text {
                anchors.centerIn: parent
                text: "green"
                color: "green"
            }
        }

        Rectangle {
            width: 12
            height: 3
            border {
                style: Border.Rounded
                color: "yellow"
            }

            Text {
                anchors.centerIn: parent
                text: "yellow"
                color: "yellow"
            }
        }
    }

    Text {
        text: "2. Indexed ANSI Palette (QmlTui.ansi(0..7))"
        color: "#8b949e"
    }

    Row {
        spacing: 1

        Repeater {
            model: 8

            Rectangle {
                required property int index
                width: 6
                height: 1
                color: QmlTui.ansi(index)

                Text {
                    anchors.centerIn: parent
                    text: "c" + parent.index
                    color: parent.index === 0 ? "white" : "black"
                    bold: true
                }
            }
        }
    }

    Text {
        text: "3. 24-Bit TrueColor RGB & QmlTui.rgba()"
        color: "#8b949e"
    }

    Row {
        spacing: 1

        Rectangle {
            width: 20
            height: 3
            color: "#ef4444"

            Text {
                anchors.centerIn: parent
                text: "#ef4444"
                color: "white"
                bold: true
            }
        }

        Rectangle {
            width: 20
            height: 3
            color: QmlTui.rgba(0.06, 0.73, 0.51, 1.0)

            Text {
                anchors.centerIn: parent
                text: "rgba(0.06,0.73…)"
                color: "black"
                bold: true
            }
        }

        Rectangle {
            width: 20
            height: 3
            color: "#3b82f6"

            Text {
                anchors.centerIn: parent
                text: "#3b82f6"
                color: "white"
                bold: true
            }
        }
    }
}
