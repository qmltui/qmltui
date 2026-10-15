pragma ComponentBehavior: Bound
import QmlTui

// Rectangle: Borders and gradients. One tile of the QmlTui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0d1117"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit();
    }

    Column {
        x: 2
        y: 1
        spacing: 1
        Row {
            spacing: 2
            Rectangle {
                width: 16
                height: 4
                border.style: Border.Single
                border.color: "#60a5fa"
                Text {
                    anchors.centerIn: parent
                    text: "single"
                    color: "#c9d1d9"
                }
            }
            Rectangle {
                width: 16
                height: 4
                border.style: Border.Double
                border.color: "#60a5fa"
                Text {
                    anchors.centerIn: parent
                    text: "double"
                    color: "#c9d1d9"
                }
            }
            Rectangle {
                width: 16
                height: 4
                border.style: Border.Rounded
                border.color: "#60a5fa"
                Text {
                    anchors.centerIn: parent
                    text: "rounded"
                    color: "#c9d1d9"
                }
            }
            Rectangle {
                width: 16
                height: 4
                border.style: Border.Ascii
                border.color: "#60a5fa"
                Text {
                    anchors.centerIn: parent
                    text: "ascii"
                    color: "#c9d1d9"
                }
            }
        }
        Row {
            spacing: 2
            Rectangle {
                width: 16
                height: 3
                color: "#1e293b"
                Text {
                    anchors.centerIn: parent
                    text: "filled"
                    color: "#c9d1d9"
                }
            }
            Rectangle {
                width: 52
                height: 3
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop {
                        position: 0.0
                        color: "#0f2027"
                    }
                    GradientStop {
                        position: 0.5
                        color: "#2c5364"
                    }
                    GradientStop {
                        position: 1.0
                        color: "#60a5fa"
                    }
                }
                Text {
                    anchors.centerIn: parent
                    text: "horizontal gradient"
                    color: "#c9d1d9"
                }
            }
        }
    }
}
