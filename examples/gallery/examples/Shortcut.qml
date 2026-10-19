pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Shortcut: Global key sequence binding that triggers regardless of which item has focus.
Column {
    id: shortcutDemo

    property string theme: "amber"
    property int refreshes: 0

    spacing: 1

    Shortcut {
        sequence: Qt.Key_1
        enabled: shortcutsOn.checked
        onActivated: shortcutDemo.theme = "amber"
    }

    Shortcut {
        sequence: Qt.Key_2
        enabled: shortcutsOn.checked
        onActivated: shortcutDemo.theme = "mint"
    }

    Shortcut {
        sequence: Qt.Key_3
        enabled: shortcutsOn.checked
        onActivated: shortcutDemo.theme = "rose"
    }

    Shortcut {
        sequence: Qt.Key_R
        enabled: shortcutsOn.checked
        onActivated: shortcutDemo.refreshes++
    }

    Text {
        text: "Shortcuts fire anywhere on the page. Press 1, 2, 3, or R."
        color: "#8b949e"
    }

    CheckBox {
        id: shortcutsOn
        text: "Shortcuts enabled"
        checked: true
        focus: true
    }

    Rectangle {
        width: 38
        height: 3
        color: shortcutDemo.theme === "amber" ? "#78350f" : shortcutDemo.theme === "mint" ? "#064e3b" : "#881337"

        Text {
            anchors.centerIn: parent
            text: "theme: " + shortcutDemo.theme + "   R pressed " + shortcutDemo.refreshes + "×"
            color: "#c9d1d9"
        }
    }
}
