pragma Singleton
import QtQml

// Boot to Qt inspired brand palette: Qt green accents on dark navy.
QtObject {
    // Brand
    readonly property string qtGreen: "#41cd52"
    readonly property string qtGreenDim: "#2ba63a"

    // Backgrounds
    readonly property string bgMain: "#09102b"
    readonly property string bgHeader: "#0d1636"
    readonly property string bgPanel: "#101c42"
    readonly property string bgActive: "#1a2a5c"

    // Borders
    readonly property string borderNormal: "#26356b"
    readonly property string borderFocus: "#41cd52"

    // Text
    readonly property string textPrimary: "#f3f3f4"
    readonly property string textSecondary: "#9d9faa"
    readonly property string textDim: "#5a6178"
    readonly property string textOnGreen: "#09102b"

    // Status
    readonly property string statusOk: "#41cd52"
    readonly property string statusWarn: "#fbbf24"
    readonly property string statusError: "#eb5757"

    // Glyphs. The framebuffer/serial console fonts (TERM=linux, vt*) cover
    // only ASCII, Latin, box drawing, and blocks — no emoji. Main.qml sets
    // simpleGlyphs from WifiManager.asciiConsole at startup.
    property bool simpleGlyphs: false

    readonly property string iconWifi: simpleGlyphs ? "" : "📶 "
    readonly property string iconScan: simpleGlyphs ? "" : "📡 "
    readonly property string iconAuth: simpleGlyphs ? "" : "🔐 "
    readonly property string iconKey: simpleGlyphs ? "" : "🔑 "
    readonly property string iconGlobe: simpleGlyphs ? "" : "🌐 "
    readonly property string iconLog: simpleGlyphs ? "" : "📋 "
    readonly property string check: simpleGlyphs ? "*" : "✔"
    readonly property string cross: simpleGlyphs ? "x" : "✖"
    readonly property string pointer: simpleGlyphs ? ">" : "▶"
    readonly property string dotOn: simpleGlyphs ? "*" : "●"
    readonly property string dotOff: simpleGlyphs ? "o" : "○"
    readonly property string keyUpDown: simpleGlyphs ? "[Up/Dn]" : "[↑/↓]"

    function makeBar(fraction, width) {
        const clamped = Math.max(0.0, Math.min(1.0, fraction))
        const filled = Math.round(clamped * width)
        return "█".repeat(filled) + "░".repeat(width - filled)
    }
}
