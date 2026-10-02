pragma Singleton
import QtQml 2.15

QtObject {
    // Semantic Palette
    readonly property string bgIdle: "#111111"
    readonly property string bgProcessing: "#0a1212"
    readonly property string bgMaintenance: "#14140e"
    readonly property string bgError: "#2a0505"
    readonly property string bgErrorAlt: "#120202"
    
    readonly property string headerBg: "#1c1e24"
    readonly property string headerBgError: "#3b0808"
    readonly property string footerBg: "#16181d"
    readonly property string footerBgError: "#3a0505"
    readonly property string cardBg: "#181a1f"
    readonly property string cardBgAlt: "#21252b"
    
    readonly property string borderNormal: "#444444"
    readonly property string borderFocus: "#e5c07b"
    readonly property string borderError: "#e06c75"
    readonly property string borderActive: "#61afef"
    
    // Status colors
    readonly property string statusIdle: "#98c379"
    readonly property string statusProcessing: "#56b6c2"
    readonly property string statusMaintenance: "#e5c07b"
    readonly property string statusError: "#e06c75"
    
    // Text colors
    readonly property string textPrimary: "#ffffff"
    readonly property string textSecondary: "#abb2bf"
    readonly property string textDim: "#5c6370"
    readonly property string textHighlight: "#61afef"
    readonly property string textWarning: "#e5c07b"
    readonly property string textDanger: "#e06c75"
    readonly property string textSuccess: "#98c379"
    readonly property string textAccent: "#c678dd"
    
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
