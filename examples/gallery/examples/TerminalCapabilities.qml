pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// TerminalCapabilities: Read-only object on Terminal reporting detected host
// features (color depth, synchronized output, Kitty keyboard/graphics, Sixel)
// and queried terminal colors (default foreground/background and ANSI palette).
Column {
    id: capDemo

    readonly property Terminal term: hostTerminal()

    function hostTerminal(): Terminal {
        let p = parent;
        while (p && p.capabilities === undefined)
            p = p.parent;
        return p as Terminal;
    }

    function formatQueriedColor(c) {
        return (c && c.valid) ? c.toString() : "unreported";
    }

    function reportedAnsiCount() {
        const t = capDemo.term;
        if (!t)
            return 0;
        let count = 0;
        for (let i = 0; i < 16; ++i) {
            if (t.capabilities.ansiColor(i).valid)
                count += 1;
        }
        return count;
    }

    function backgroundSpans() {
        const t = capDemo.term;
        const c = t ? t.capabilities.defaultBackgroundColor : null;
        return [
            {
                text: "Default Background:  "
            },
            {
                text: "██",
                color: (c && c.valid) ? c : QmlTui.defaultColor
            },
            {
                text: " " + capDemo.formatQueriedColor(c)
            }
        ];
    }

    function paletteSpans() {
        const t = capDemo.term;
        let list = [
            {
                text: "ANSI Palette (" + capDemo.reportedAnsiCount() + "/16 reported):\n",
                color: "#c9d1d9"
            }
        ];
        for (let i = 0; i < 16; ++i) {
            const ansiColor = QmlTui.ansi(i);
            const c = t ? t.capabilities.ansiColor(i) : null;
            const name = ansiColor.toString().replace(/^ansi:/, "");
            list.push({
                text: "██ ",
                color: (c && c.valid) ? c : ansiColor
            });
            list.push({
                text: name.padEnd(13, " ") + ((i % 4 === 3 && i < 15) ? "\n" : " "),
                color: "#c9d1d9"
            });
        }
        return list;
    }

    function refreshCapabilities() {
        defaultBgText.spans = capDemo.backgroundSpans();
        paletteText.spans = capDemo.paletteSpans();
    }

    anchors.centerIn: parent
    spacing: 1

    onTermChanged: {
        if (capDemo.term)
            capDemo.term.queryColors();
        capDemo.refreshCapabilities();
    }
    Component.onCompleted: capDemo.refreshCapabilities()

    Keys.onPressed: event => {
        if (event.key === Qt.Key_R && capDemo.term) {
            capDemo.term.queryColors();
            event.accepted = true;
        }
    }

    Connections {
        target: capDemo.term
        function onCapabilitiesChanged() {
            capDemo.refreshCapabilities();
        }
    }

    Text {
        text: "Host Terminal Feature Detection & Diagnostics"
        color: "#38bdf8"
        bold: true
    }

    Column {
        spacing: 0

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
            text: "Default Foreground:  " + "██ " + (capDemo.term ? capDemo.formatQueriedColor(capDemo.term.capabilities.defaultForegroundColor) : "unreported")
            color: (capDemo.term && capDemo.term.capabilities.defaultForegroundColor.valid) ? capDemo.term.capabilities.defaultForegroundColor : QmlTui.defaultColor
        }

        Text {
            id: defaultBgText
        }

        Text {
            id: paletteText
        }
    }
}
