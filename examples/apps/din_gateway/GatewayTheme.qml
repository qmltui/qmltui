pragma Singleton
import QtQml

QtObject {
    // Background Colors
    readonly property string bgMain: "#05070a"
    readonly property string bgHeader: "#0f172a"
    readonly property string bgFooter: "#0f172a"
    readonly property string bgPanel: "#0b1120"
    readonly property string bgCard: "#161f33"
    readonly property string bgHighlight: "#1e293b"
    readonly property string bgActiveTab: "#0284c7"

    // Border Colors
    readonly property string borderNormal: "#334155"
    readonly property string borderFocus: "#38bdf8"
    readonly property string borderActive: "#0284c7"
    readonly property string borderWarning: "#f59e0b"
    readonly property string borderDanger: "#ef4444"

    // Status Indicator Colors
    readonly property string statusOk: "#10b981"
    readonly property string statusWarn: "#f59e0b"
    readonly property string statusError: "#ef4444"
    readonly property string statusInfo: "#38bdf8"

    // Text Colors
    readonly property string textPrimary: "#f8fafc"
    readonly property string textSecondary: "#94a3b8"
    readonly property string textDim: "#64748b"
    readonly property string textAccent: "#38bdf8"
    readonly property string textSuccess: "#34d399"
    readonly property string textWarning: "#fbbf24"
    readonly property string textDanger: "#f87171"

    // Helper method to generate creative visual progress meters
    function makeProgressBar(val, from, to, width, style) {
        if (width <= 0) return "";
        let pct = Math.max(0.0, Math.min(1.0, (val - from) / (to - from)));
        let fillLen = Math.round(pct * width);
        let emptyLen = width - fillLen;

        let fillChar = "▰";
        let emptyChar = "▱";

        if (style === "pill") {
            fillChar = "▮";
            emptyChar = "▯";
        } else if (style === "dot") {
            fillChar = "●";
            emptyChar = "○";
        } else if (style === "shade") {
            let str = "";
            for (let i = 0; i < fillLen; i++) {
                if (i === fillLen - 1 && fillLen < width) str += "▓";
                else str += "█";
            }
            for (let j = 0; j < emptyLen; j++) {
                if (j === 0 && fillLen > 0) str += "▒";
                else str += "░";
            }
            return str;
        }

        let filled = "";
        for (let i = 0; i < fillLen; i++) filled += fillChar;
        let empty = "";
        for (let j = 0; j < emptyLen; j++) empty += emptyChar;
        return filled + empty;
    }
}
