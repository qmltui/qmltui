pragma Singleton
import QtQml

QtObject {
    // Theme: Catppuccin Mocha / Tokyo Night Dark Palette
    readonly property string bgMain:       "#1e1e2e"
    readonly property string bgHeader:     "#181825"
    readonly property string bgPanel:      "#181825"
    readonly property string bgColumn:     "#1e1e2e"
    readonly property string bgColumnDim:  "#181825"
    readonly property string bgCursor:     "#313244"
    readonly property string bgSelected:   "#45475a"
    readonly property string bgMarked:     "#3b4261"
    readonly property string bgModal:      "#181825"

    // Borders & Dividers
    readonly property string borderNormal: "#313244"
    readonly property string borderActive: "#89b4fa"
    readonly property string borderMuted:  "#45475a"
    readonly property string borderYellow: "#f9e2af"
    readonly property string borderGreen:  "#a6e3a1"
    readonly property string borderRed:    "#f38ba8"

    // Foreground / Text
    readonly property string textPrimary:   "#cdd6f4"
    readonly property string textSecondary: "#a6adc8"
    readonly property string textMuted:     "#6c7086"
    readonly property string textDim:       "#585b70"
    readonly property string textHighlight: "#ffffff"

    // Accent Colors
    readonly property string accentBlue:   "#89b4fa"
    readonly property string accentCyan:   "#89dceb"
    readonly property string accentGreen:  "#a6e3a1"
    readonly property string accentYellow: "#f9e2af"
    readonly property string accentPeach:  "#fab387"
    readonly property string accentMauve:  "#cba6f7"
    readonly property string accentRed:    "#f38ba8"
    readonly property string accentPink:   "#f5c2e7"

    // Status Badges
    readonly property string badgeNormal:  "#89b4fa"
    readonly property string badgeSelect:  "#f9e2af"
    readonly property string badgeFilter:  "#f5c2e7"
    readonly property string badgeCommand: "#fab387"

    // Glyphs
    readonly property string iconFolder:   "📁"
    readonly property string iconFile:     "📄"
    readonly property string iconCpp:      "⚡"
    readonly property string iconRust:     "🦀"
    readonly property string iconPython:   "🐍"
    readonly property string iconConfig:   "⚙️"
    readonly property string iconArchive:  "📦"
    readonly property string iconImage:    "🖼️"
    readonly property string iconCheck:    "✓"
    readonly property string iconCursor:   "▸"
    readonly property string iconDisk:     "💾"
    readonly property string iconUser:     "👤"
}
