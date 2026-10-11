pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Controls

// The default look of the button-like controls, pinned by a styled snapshot
// so the AbstractButton template and later restyling keep every cell and
// attribute of today's rendering: focused and plain Button at implicit size,
// a one-row Button (no border, bold on focus), a disabled Button, and the
// three toggle controls with and without focus.
Terminal {
    id: root
    Button { id: first; x: 1; y: 1; text: "Focused"; focus: true }
    Button { x: 16; y: 1; text: "Plain" }
    Button { x: 29; y: 2; height: 1; text: "OneRow" }
    Button { x: 42; y: 1; text: "Off"; enabled: false }
    CheckBox { x: 1; y: 5; text: "Logging"; checked: true }
    CheckBox { x: 16; y: 5; text: "Verbose" }
    ButtonGroup { buttons: [fast, safe, slow] }
    RadioButton { id: fast; x: 1; y: 7; text: "Fast"; checked: true }
    RadioButton { id: safe; x: 16; y: 7; text: "Safe" }
    RadioButton { id: slow; x: 29; y: 7; text: "Slow" }
    Switch { x: 1; y: 9; text: "Wi-Fi"; checked: true }
    Switch { x: 16; y: 9; text: "Bluetooth" }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q) Qt.quit()
    }
}
