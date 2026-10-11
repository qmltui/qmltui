pragma ComponentBehavior: Bound

import QtQml
import Qamui

import Qamui.Controls
Popup {
    id: root

    modal: true
    dimBackground: true
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: parent
    width: 62
    height: 18
    color: Theme.bgModal
    border.style: "double"
    border.color: Theme.accentYellow

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Title
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 1
            Text {
                text: "📖"
                color: Theme.accentYellow
            }
            Text {
                text: "YAZI FILE MANAGER — KEYBINDINGS"
                color: Theme.accentYellow
                bold: true
            }
        }

        // Section 1: Navigation
        Column {
            width: parent.width
            spacing: 0

            Text {
                text: "── NAVIGATION ──────────────────────────────────────────"
                color: Theme.accentBlue
                bold: true
            }
            Row {
                spacing: 2
                Text { width: 14; text: " h / Left"; color: Theme.accentCyan; bold: true }
                Text { text: "Parent directory (cd ..)"; color: Theme.textPrimary }
            }
            Row {
                spacing: 2
                Text { width: 14; text: " l / Right / ↵"; color: Theme.accentCyan; bold: true }
                Text { text: "Enter directory / open file"; color: Theme.textPrimary }
            }
            Row {
                spacing: 2
                Text { width: 14; text: " j / Down"; color: Theme.accentCyan; bold: true }
                Text { text: "Move cursor down"; color: Theme.textPrimary }
            }
            Row {
                spacing: 2
                Text { width: 14; text: " k / Up"; color: Theme.accentCyan; bold: true }
                Text { text: "Move cursor up"; color: Theme.textPrimary }
            }
            Row {
                spacing: 2
                Text { width: 14; text: " g / G"; color: Theme.accentCyan; bold: true }
                Text { text: "Jump to first / last item"; color: Theme.textPrimary }
            }
        }

        // Section 2: Operations
        Column {
            width: parent.width
            spacing: 0

            Text {
                text: "── SELECTION & ACTIONS ─────────────────────────────────"
                color: Theme.accentGreen
                bold: true
            }
            Row {
                spacing: 2
                Text { width: 14; text: " Space / v"; color: Theme.accentGreen; bold: true }
                Text { text: "Toggle mark selection"; color: Theme.textPrimary }
            }
            Row {
                spacing: 2
                Text { width: 14; text: " V / Ctrl+A"; color: Theme.accentGreen; bold: true }
                Text { text: "Select all / Invert selection"; color: Theme.textPrimary }
            }
            Row {
                spacing: 2
                Text { width: 14; text: " y / x / p"; color: Theme.accentGreen; bold: true }
                Text { text: "Yank (copy) / Cut / Paste"; color: Theme.textPrimary }
            }
            Row {
                spacing: 2
                Text { width: 14; text: " a / r / d"; color: Theme.accentGreen; bold: true }
                Text { text: "New file-dir / Rename / Delete"; color: Theme.textPrimary }
            }
            Row {
                spacing: 2
                Text { width: 14; text: " / / f"; color: Theme.accentGreen; bold: true }
                Text { text: "Instant live filter mode"; color: Theme.textPrimary }
            }
            Row {
                spacing: 2
                Text { width: 14; text: " . / s"; color: Theme.accentGreen; bold: true }
                Text { text: "Toggle dotfiles / Cycle sort mode"; color: Theme.textPrimary }
            }
        }

        // Footer
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 1
            Text { text: "Press "; color: Theme.textDim }
            Text { text: "[Esc]"; color: Theme.accentYellow; bold: true }
            Text { text: " or "; color: Theme.textDim }
            Text { text: "[?]"; color: Theme.accentYellow; bold: true }
            Text { text: " to close"; color: Theme.textDim }
        }
    }
}
