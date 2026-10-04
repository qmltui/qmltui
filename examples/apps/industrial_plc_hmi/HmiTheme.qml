pragma Singleton
import QtQml

QtObject {
    id: root
    // Semantic Palette (Stitch Design Tokens: Cyber-Industrial Terminal)
    readonly property string bgIdle: "#131314"
    readonly property string bgProcessing: "#0d1a0e"
    readonly property string bgMaintenance: "#1f180d"
    readonly property string bgError: "#2a0505"
    readonly property string bgErrorAlt: "#120202"

    readonly property string headerBg: "#1f2020"
    readonly property string headerBgError: "#3b0808"
    readonly property string footerBg: "#1b1c1c"
    readonly property string footerBgError: "#3a0505"
    readonly property string cardBg: "#1f2020"
    readonly property string cardBgAlt: "#2a2a2a"

    readonly property string borderNormal: "#3b4b37"
    readonly property string borderFocus: "#00ff41"
    readonly property string borderError: "#ff3e3e"
    readonly property string borderActive: "#00ff41"

    // Status colors
    readonly property string statusIdle: "#00ff41"
    readonly property string statusProcessing: "#00ff41"
    readonly property string statusMaintenance: "#ffd393"
    readonly property string statusError: "#ff3e3e"

    // Text colors
    readonly property string textPrimary: "#e4e2e2"
    readonly property string textSecondary: "#b9ccb2"
    readonly property string textDim: "#84967e"
    readonly property string textHighlight: "#00ff41"
    readonly property string textWarning: "#ffd393"
    readonly property string textDanger: "#ff3e3e"
    readonly property string textSuccess: "#00ff41"
    readonly property string textAccent: "#ffd393"
    
    // Progress bar helper
    function makeProgressBar(value, minVal, maxVal, width) {
        let pct = Math.max(0.0, Math.min(1.0, (value - minVal) / (maxVal - minVal)));
        let filled = Math.round(pct * width);
        let empty = width - filled;
        let bar = "";
        for (let i = 0; i < filled; i++) bar += "█";
        for (let j = 0; j < empty; j++) bar += "░";
        return bar;
    }
}
