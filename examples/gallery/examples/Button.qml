pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Button: Push, checkable, and custom-styled buttons.
// Space or Enter activates the focused button; Tab moves focus between buttons.
Column {
    id: buttonDemo

    property int clicks: 0

    spacing: 1

    Text {
        text: "Space/Enter or click triggers onClicked."
        color: "#8b949e"
    }

    // Push buttons with Single/Double, Rounded, and borderless Filled styles
    Row {
        id: buttons
        spacing: 1

        Button {
            text: "Primary"
            focus: true
            hoverEnabled: true
            onClicked: buttonDemo.clicks++
        }

        Button {
            id: secondary
            text: "Secondary"
            hoverEnabled: true
            background: Rectangle {
                color: "default"
                border {
                    color: secondary.activeFocus ? "#2cde85" : "#60a5fa"
                    style: secondary.activeFocus ? Border.Double : Border.Rounded
                }
            }
            onClicked: buttonDemo.clicks++
        }

        Button {
            id: filled
            y: 1
            text: "Filled"
            textColor: "#0d1117"
            hoverEnabled: true
            background: Rectangle {
                color: filled.activeFocus ? "#00ff66" : "#2cde85"
                border {
                    color: "default"
                    style: Border.None
                }
            }
            onClicked: buttonDemo.clicks++
        }
    }

    // Checkable toggle button, custom contentItem button, and Ascii-bordered Reset
    Row {
        spacing: 1

        Button {
            id: pin
            checkable: true
            hoverEnabled: true
            highlighted: pin.checked
            text: pin.checked ? "★ Pinned" : "☆ Pin"
            textColor: pin.checked ? "#0d1117" : (pin.activeFocus ? "#2cde85" : "default")
            background: Rectangle {
                color: pin.checked ? "#2cde85" : "default"
                border {
                    color: pin.activeFocus ? (pin.checked ? "#0d1117" : "#2cde85") : (pin.checked ? "#1f8a52" : "#30363d")
                    style: pin.activeFocus ? Border.Double : Border.Single
                }
            }
        }

        Button {
            id: withIcon
            text: "Save"
            hoverEnabled: true
            background: Rectangle {
                color: "default"
                border {
                    color: withIcon.activeFocus ? "#7ee0ff" : "#30363d"
                    style: withIcon.activeFocus ? Border.Double : Border.Rounded
                }
            }
            contentItem: Row {
                spacing: 1

                Text {
                    text: "◆"
                    color: "#7ee0ff"
                }

                Text {
                    text: withIcon.text
                    color: "#7ee0ff"
                    bold: withIcon.activeFocus || withIcon.hovered
                }
            }
            onClicked: buttonDemo.clicks++
        }

        Button {
            id: reset
            text: "Reset"
            textColor: reset.activeFocus ? "#ffdcd7" : "#f87171"
            hoverEnabled: true
            background: Rectangle {
                color: reset.activeFocus ? "#3d1518" : "default"
                border {
                    color: reset.activeFocus ? "#ff7b72" : "#8b3a3a"
                    style: Border.Ascii
                }
            }
            onClicked: buttonDemo.clicks = 0
        }
    }

    Text {
        text: buttonDemo.clicks === 0 ? "No clicks yet" : "Clicked " + buttonDemo.clicks + " time" + (buttonDemo.clicks === 1 ? "" : "s")
        color: "#2cde85"
        bold: true
    }
}
