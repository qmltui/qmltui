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
    property color fgColor: QmlTui.defaultColor
    property color bgColor: QmlTui.defaultColor
    property color accentCyan: QmlTui.ansi(14)
    property color accentGreen: QmlTui.ansi(10)
    property string termProgramValue: "unknown"
    property string colorDepthValue: "unknown"
    property bool syncOutputSupported: false
    property bool kittyKeyboardSupported: false
    property bool kittyGraphicsSupported: false
    property bool sixelGraphicsSupported: false

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

    function foregroundSpans() {
        const t = capDemo.term;
        const c = t ? t.capabilities.defaultForegroundColor : null;
        return [
            {
                text: "Default Foreground:  ",
                color: capDemo.fgColor
            },
            {
                text: "██",
                color: (c && c.valid) ? c : QmlTui.defaultColor
            },
            {
                text: " " + capDemo.formatQueriedColor(c),
                color: capDemo.fgColor
            }
        ];
    }

    function backgroundSpans() {
        const t = capDemo.term;
        const c = t ? t.capabilities.defaultBackgroundColor : null;
        return [
            {
                text: "Default Background:  ",
                color: capDemo.fgColor
            },
            {
                text: "██",
                color: (c && c.valid) ? c : QmlTui.defaultColor
            },
            {
                text: " " + capDemo.formatQueriedColor(c),
                color: capDemo.fgColor
            }
        ];
    }

    function paletteSpans() {
        const t = capDemo.term;
        let list = [
            {
                text: "ANSI Palette (" + capDemo.reportedAnsiCount() + "/16 reported):\n",
                color: capDemo.fgColor
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
                color: capDemo.fgColor
            });
        }
        return list;
    }

    function refreshCapabilities() {
        const t = capDemo.term;
        const fg = t ? t.capabilities.defaultForegroundColor : null;
        const bg = t ? t.capabilities.defaultBackgroundColor : null;
        const cyan = t ? (t.capabilities.ansiColor(14).valid ? t.capabilities.ansiColor(14) : t.capabilities.ansiColor(6)) : null;
        const green = t ? (t.capabilities.ansiColor(10).valid ? t.capabilities.ansiColor(10) : t.capabilities.ansiColor(2)) : null;

        capDemo.fgColor = (fg && fg.valid) ? fg : QmlTui.defaultColor;
        capDemo.bgColor = (bg && bg.valid) ? bg : QmlTui.defaultColor;
        capDemo.accentCyan = (cyan && cyan.valid) ? cyan : QmlTui.ansi(14);
        capDemo.accentGreen = (green && green.valid) ? green : QmlTui.ansi(10);

        capDemo.termProgramValue = (t && t.capabilities.termProgram) ? t.capabilities.termProgram : "unknown";
        capDemo.colorDepthValue = t ? t.capabilities.colorDepth : "unknown";
        capDemo.syncOutputSupported = Boolean(t && t.capabilities.syncOutput);
        capDemo.kittyKeyboardSupported = Boolean(t && t.capabilities.kittyKeyboard);
        capDemo.kittyGraphicsSupported = Boolean(t && t.capabilities.kittyGraphics);
        capDemo.sixelGraphicsSupported = Boolean(t && t.capabilities.sixelGraphics);

        defaultFgText.spans = capDemo.foregroundSpans();
        defaultBgText.spans = capDemo.backgroundSpans();
        paletteText.spans = capDemo.paletteSpans();
    }

    anchors.centerIn: parent
    spacing: 1

    Component.onCompleted: {
        if (capDemo.term)
            capDemo.term.queryColors();
        capDemo.refreshCapabilities();
    }

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
        color: capDemo.accentCyan
        bold: true
    }

    Column {
        spacing: 0

        // Read capabilities from the owning Terminal instance
        Text {
            text: "Terminal Program:    " + capDemo.termProgramValue
            color: capDemo.fgColor
        }

        Text {
            text: "Color Resolution:    " + capDemo.colorDepthValue
            color: capDemo.accentGreen
        }

        Text {
            text: "Synchronized Output: " + (capDemo.syncOutputSupported ? "Supported" : "Unsupported")
            color: capDemo.fgColor
        }

        Text {
            text: "Kitty Keyboard:      " + (capDemo.kittyKeyboardSupported ? "Supported" : "Unsupported")
            color: capDemo.fgColor
        }

        Text {
            text: "Kitty Graphics:      " + (capDemo.kittyGraphicsSupported ? "Supported" : "Unsupported")
            color: capDemo.fgColor
        }

        Text {
            text: "Sixel Graphics:      " + (capDemo.sixelGraphicsSupported ? "Supported" : "Unsupported")
            color: capDemo.fgColor
        }

        Text {
            id: defaultFgText
        }

        Text {
            id: defaultBgText
        }

        Text {
            id: paletteText
        }
    }
}
