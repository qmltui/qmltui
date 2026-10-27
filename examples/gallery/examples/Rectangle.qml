pragma ComponentBehavior: Bound
import QmlTui

// Rectangle: Colored cell surfaces, four box-drawing border styles, and gradients.
Column {
    anchors.centerIn: parent
    spacing: 1

    // Four built-in border styles: Single, Double, Rounded, and Ascii
    Row {
        spacing: 1

        Rectangle {
            width: 10
            height: 3
            border {
                style: Border.Single
                color: "#60a5fa"
            }

            Text {
                anchors.centerIn: parent
                text: "single"
                color: "#c9d1d9"
            }
        }

        Rectangle {
            width: 10
            height: 3
            border {
                style: Border.Double
                color: "#60a5fa"
            }

            Text {
                anchors.centerIn: parent
                text: "double"
                color: "#c9d1d9"
            }
        }

        Rectangle {
            width: 10
            height: 3
            border {
                style: Border.Rounded
                color: "#60a5fa"
            }

            Text {
                anchors.centerIn: parent
                text: "rounded"
                color: "#c9d1d9"
            }
        }

        Rectangle {
            width: 10
            height: 3
            border {
                style: Border.Ascii
                color: "#60a5fa"
            }

            Text {
                anchors.centerIn: parent
                text: "ascii"
                color: "#c9d1d9"
            }
        }
    }

    // Single 1-height solid background fill and horizontal Gradient bars
    Row {
        spacing: 1

        Rectangle {
            width: 10
            height: 1
            color: "#1e3a5f"

            Text {
                anchors.centerIn: parent
                text: "filled"
                color: "#c9d1d9"
            }
        }

        Rectangle {
            width: 32
            height: 1
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

    // Full-width single 1-height status bar Rectangle
    Rectangle {
        width: 43
        height: 1
        color: "#2cde85"

        Text {
            anchors.centerIn: parent
            text: "1-row status bar (height: 1)"
            color: "#0b1911"
            bold: true
        }
    }
}
