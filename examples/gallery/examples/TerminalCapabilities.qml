pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// TerminalCapabilities: Read-only object on Terminal reporting detected host
// features (color depth, synchronized output, Kitty keyboard/graphics, Sixel).
Column {
    id: capDemo

    property int reportCount: 0
    readonly property Terminal term: hostTerminal()

    function hostTerminal(): Terminal {
        let p = parent;
        while (p && p.capabilities === undefined)
            p = p.parent;
        return p as Terminal;
    }

    function printCapabilities() {
        const t = capDemo.term;
        history.print("Program: " + (t && t.capabilities.termProgram ? t.capabilities.termProgram : "unknown") + " | Color: " + (t ? t.capabilities.colorDepth : "unknown"));
        capDemo.reportCount += 1;
    }

    spacing: 1

    onTermChanged: {
        if (capDemo.term && capDemo.reportCount === 0)
            capDemo.printCapabilities();
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_P) {
            capDemo.printCapabilities();
            event.accepted = true;
        }
    }

    Scrollback {
        id: history
    }

    Text {
        text: "Host Terminal Feature Detection & Diagnostics"
        color: "#38bdf8"
        bold: true
    }

    // Read capabilities from the owning Terminal instance
    Text {
        text: "Terminal Program:    " + ((capDemo.term && capDemo.term.capabilities.termProgram) ? capDemo.term.capabilities.termProgram : "unknown")
        color: "#c9d1d9"
    }

    Text {
        text: "Color Resolution:    " + (capDemo.term ? capDemo.term.capabilities.colorDepth : "unknown")
        color: "#2cde85"
    }

    Text {
        text: "Synchronized Output: " + ((capDemo.term && capDemo.term.capabilities.syncOutput) ? "Supported" : "Unsupported")
        color: "#c9d1d9"
    }

    Text {
        text: "Kitty Keyboard:      " + ((capDemo.term && capDemo.term.capabilities.kittyKeyboard) ? "Supported" : "Unsupported")
        color: "#c9d1d9"
    }

    Text {
        text: "Kitty Graphics:      " + ((capDemo.term && capDemo.term.capabilities.kittyGraphics) ? "Supported" : "Unsupported")
        color: "#c9d1d9"
    }

    Text {
        text: "Sixel Graphics:      " + ((capDemo.term && capDemo.term.capabilities.sixelGraphics) ? "Supported" : "Unsupported")
        color: "#c9d1d9"
    }

    Text {
        text: "Reports Printed: " + capDemo.reportCount
        color: "#fbbf24"
        bold: true
    }

    Button {
        focus: true
        text: "Print Report (P)"
        textColor: "#38bdf8"
        onClicked: capDemo.printCapabilities()
    }
}
