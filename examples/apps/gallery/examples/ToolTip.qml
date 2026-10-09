pragma ComponentBehavior: Bound
import Qtui
import Qtui.Controls

// ToolTip: Attached hover/focus tip. One tile of the Qtui gallery (examples/apps/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0b0f14"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Column {
        x: 2
        y: 1
        spacing: 1
        Text { text: "A tip appears after `delay` ms of hover or focus; `visible: true` pins one."; color: "#6e7b8b" }
        Row {
            spacing: 3
            Button { text: "Build"; focus: true; ToolTip.text: "Compile the current project"; ToolTip.delay: 300 }
            Button { text: "Deploy"; ToolTip.text: "Push the build to staging"; ToolTip.textColor: "#fda4af" }
            Button { text: "Pinned"; ToolTip.text: "This tip is pinned with ToolTip.visible"; ToolTip.visible: true; ToolTip.timeout: 0 }
        }
        Text { text: ""; height: 3 }
        Text { text: "Tab between the buttons and wait a moment, or hover with the mouse."; color: "#fda4af" }
    }
}
