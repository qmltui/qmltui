pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// Keys: Attached Keys.onPressed handler and KeyEvent inspection.
Column {
    id: keysDemo

    property string lastKeyName: "(press any key)"
    property string lastText: "—"
    property string lastMods: "None"
    property int markerX: 20
    property int markerY: 3

    anchors.centerIn: parent
    spacing: 1
    focusPolicy: Qt.StrongFocus
    focus: true

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Q)
            return;

        const mods = [];
        if (event.modifiers & Qt.ControlModifier)
            mods.push("Ctrl");
        if (event.modifiers & Qt.AltModifier)
            mods.push("Alt");
        if (event.modifiers & Qt.ShiftModifier)
            mods.push("Shift");
        keysDemo.lastMods = mods.length > 0 ? mods.join("+") : "None";
        keysDemo.lastText = event.text !== "" ? "\"" + event.text + "\"" : "(non-printable)";

        if (event.key === Qt.Key_Left) {
            keysDemo.markerX = Math.max(1, keysDemo.markerX - 2);
            keysDemo.lastKeyName = "Qt.Key_Left";
        } else if (event.key === Qt.Key_Right) {
            keysDemo.markerX = Math.min(38, keysDemo.markerX + 2);
            keysDemo.lastKeyName = "Qt.Key_Right";
        } else if (event.key === Qt.Key_Up) {
            keysDemo.markerY = Math.max(1, keysDemo.markerY - 1);
            keysDemo.lastKeyName = "Qt.Key_Up";
        } else if (event.key === Qt.Key_Down) {
            keysDemo.markerY = Math.min(5, keysDemo.markerY + 1);
            keysDemo.lastKeyName = "Qt.Key_Down";
        } else if (event.key === Qt.Key_Space) {
            keysDemo.lastKeyName = "Qt.Key_Space";
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            keysDemo.lastKeyName = "Qt.Key_Return";
        } else {
            keysDemo.lastKeyName = "0x" + event.key.toString(16);
        }
        event.accepted = true;
    }

    Text {
        text: "Press arrow keys to move the marker, or type any key to inspect KeyEvent:"
        color: "#8b949e"
    }

    Rectangle {
        width: 42
        height: 7
        border {
            style: Border.Rounded
            color: keysDemo.activeFocus ? "#2cde85" : "#30363d"
        }

        Text {
            x: keysDemo.markerX
            y: keysDemo.markerY
            text: "◆"
            color: "#00ff66"
            bold: true
        }
    }

    Text {
        text: "key: " + keysDemo.lastKeyName + "  ·  text: " + keysDemo.lastText + "  ·  modifiers: " + keysDemo.lastMods
        color: "#58a6ff"
        bold: true
    }
}
