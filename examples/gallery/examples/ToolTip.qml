pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// ToolTip: Attached property displaying contextual help on focus or hover.
Column {
    spacing: 1

    Text {
        text: "A tip appears after `delay` ms of hover or focus; `visible: true` pins one."
        color: "#8b949e"
    }

    Row {
        spacing: 2

        Button {
            text: "Build"
            focus: true
            ToolTip.text: "Compile the current project"
            ToolTip.delay: 300
        }

        Button {
            text: "Deploy"
            ToolTip.text: "Push the build to staging"
            ToolTip.delay: 300
            ToolTip.textColor: "#fda4af"
        }

        Button {
            text: "Pinned"
            ToolTip.text: "This tip is pinned with ToolTip.visible"
            ToolTip.visible: true
            ToolTip.timeout: 0
        }
    }

    Item {
        width: 1
        height: 3
    }

    Text {
        text: "Tab between the buttons and wait a moment, or hover with the mouse."
        color: "#fda4af"
    }
}
