pragma Singleton
import QtQml

QtObject {
    // Backgrounds
    readonly property string bgMain: "#020617"
    readonly property string bgHeader: "#0f172a"
    readonly property string bgPanel: "#0b1120"
    readonly property string bgActiveTab: "#047857"

    // Borders
    readonly property string borderNormal: "#334155"
    readonly property string borderFocus: "#34d399"

    // Status
    readonly property string statusOk: "#10b981"
    readonly property string statusWarn: "#f59e0b"
    readonly property string statusError: "#ef4444"

    // Text
    readonly property string textPrimary: "#f8fafc"
    readonly property string textSecondary: "#94a3b8"
    readonly property string textDim: "#64748b"
    readonly property string textAccent: "#34d399"
    readonly property string textWarning: "#fbbf24"
    readonly property string textDanger: "#f87171"

    function formatUptime(seconds) {
        const days = Math.floor(seconds / 86400)
        const hours = Math.floor((seconds % 86400) / 3600)
        const minutes = Math.floor((seconds % 3600) / 60)
        return (days > 0 ? days + "d " : "") + hours + "h " + minutes + "m"
    }

    function formatMb(mb) {
        return mb >= 1024 ? (mb / 1024).toFixed(1) + " GB" : mb + " MB"
    }

    function makeBar(fraction, width) {
        const clamped = Math.max(0.0, Math.min(1.0, fraction))
        const filled = Math.round(clamped * width)
        return "█".repeat(filled) + "░".repeat(width - filled)
    }
}
