pragma ComponentBehavior: Bound
import Qamui
import Qamui.Controls

// Shortcut: Global key without focus. One tile of the Qamui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0b0f14"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        id: shortcutDemo
        x: 2
        y: 1
        property string theme: "amber"
        property int refreshes: 0
        spacing: 1
        Shortcut { sequence: Qt.Key_1; enabled: shortcutsOn.checked; onActivated: shortcutDemo.theme = "amber" }
        Shortcut { sequence: Qt.Key_2; enabled: shortcutsOn.checked; onActivated: shortcutDemo.theme = "mint" }
        Shortcut { sequence: Qt.Key_3; enabled: shortcutsOn.checked; onActivated: shortcutDemo.theme = "rose" }
        Shortcut { sequence: Qt.Key_F5; enabled: shortcutsOn.checked; onActivated: shortcutDemo.refreshes++ }
        Text { text: "Shortcuts fire from anywhere on the page, whatever has focus. Press 1, 2, 3 or F5."; color: "#6e7b8b" }
        CheckBox { id: shortcutsOn; text: "Shortcuts enabled"; checked: true; focus: true }
        Rectangle {
            width: 40
            height: 3
            color: shortcutDemo.theme === "amber" ? "#78350f" : shortcutDemo.theme === "mint" ? "#064e3b" : "#881337"
            Text { anchors.centerIn: parent; text: "theme: " + shortcutDemo.theme + "   F5 pressed " + shortcutDemo.refreshes + "×"; color: "#c9d1d9" }
        }
    }
}
