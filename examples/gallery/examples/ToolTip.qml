pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// ToolTip: Attached contextual help supporting backgroundColor, border,
// and custom background / contentItem slots.
Column {
    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "A tip appears after `delay` ms of hover or focus; `visible: true` pins one."
        color: "#8b949e"
    }

    Row {
        spacing: 4

        Button {
            text: "Build"
            focus: true
            ToolTip.text: "Compile the current project"
            ToolTip.delay: 250
            ToolTip.backgroundColor: "#161b22"
            ToolTip.textColor: "#79c0ff"
        }

        Button {
            text: "Deploy"
            ToolTip.text: "Push the build to staging"
            ToolTip.delay: 250
            ToolTip.backgroundColor: "#0d1117"
            ToolTip.textColor: "#00ff66"
            ToolTip.border {
                style: Border.Rounded
                color: "#2cde85"
            }
        }
    }

    Item {
        width: 1
        height: 2
    }

    Button {
        text: "Pinned"
        ToolTip.visible: true
        ToolTip.timeout: 0
        ToolTip.background: Rectangle {
            color: "#0d1117"
            border {
                style: Border.Single
                color: "#f2cc60"
            }
        }
        ToolTip.contentItem: Row {
            spacing: 1

            Text {
                text: "★"
                color: "#f2cc60"
                bold: true
            }

            Text {
                text: "This tip is pinned with ToolTip.visible"
                color: "#c9d1d9"
                bold: true
            }
        }
    }

    Item {
        width: 1
        height: 2
    }

    Text {
        text: "Tab between the buttons and wait a moment, or hover with the mouse."
        color: "#2cde85"
    }
}
