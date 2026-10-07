pragma Singleton
import QtQml

// Neon palette and small formatting helpers shared by every cyber_audio component.
QtObject {
    // Backgrounds
    readonly property string background: "#05050a"
    readonly property string barBackground: "#0c101c"
    readonly property string transportBackground: "#0a0d18"
    readonly property string panel: "#080b14"
    readonly property string frame: "#1e293b"

    // Text
    readonly property string text: "#e2e8f0"
    readonly property string subtle: "#94a3b8"
    readonly property string muted: "#64748b"
    readonly property string dim: "#475569"

    // Semantic colors
    readonly property string mid: "#a855f7"          // upper-middle equalizer rows
    readonly property string error: "#ff003c"
    readonly property string warning: "#eab308"
    readonly property string hot: "#ef4444"          // clipping VU
    readonly property string playing: "#22c55e"
    readonly property string fileSource: "#22c55e"
    readonly property string micSource: "#f59e0b"
    readonly property string equalizerTitle: "#38bdf8"

    // Each chart keeps its own hue so the two panels never share a color.
    readonly property string accent: "#00f0ff"                   // equalizer bars, UI chrome (cyan)
    readonly property string secondary: "#ff0055"                // equalizer tops, beat, VU heading (pink)
    readonly property string scopeAccent: "#ffb700"              // left scope trace (amber)
    readonly property string scopeRight: "#ff4fd8"               // right scope trace (magenta)
    readonly property string vuLow: "#22c55e"                    // classic green / amber / red meter

    // 1/8-cell vertical resolution for smooth bar tops.
    readonly property var blockGlyphs: [" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"]

    // Truncates text to maxCells, ending with an ellipsis when cut.
    function fitText(text, maxCells) {
        if (maxCells <= 0)
            return ""
        if (text.length <= maxCells)
            return text
        return text.substring(0, Math.max(0, maxCells - 1)) + "…"
    }

    // Level 0..1 to a dB string with a full-scale reference of 1.0.
    function formatDecibels(level) {
        if (level <= 0.01)
            return "-∞ dB"
        const db = 20 * Math.log10(level)
        return (db >= 0 ? "+" : "") + db.toFixed(1) + " dB"
    }
}
