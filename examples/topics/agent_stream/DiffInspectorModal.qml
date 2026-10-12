import QtQml
import Qamui
import Qamui.Controls

Popup {
    id: diffModal

    Theme { id: theme }

    property string targetFile: "src/Qamui/engine/StreamRenderer.cpp"

    width: parent ? parent.width : 78
    height: 13
    clip: true
    modal: true
    dimBackground: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnOutsideClick

    Rectangle {
        anchors.fill: parent
        color: "#0f172a"
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        // Header Title
        Row {
            width: parent.width
            height: 1
            spacing: 1

            StatusBadge {
                text: "Diff"
                status: "warning"
                variant: "subtle"
                showIcon: false
            }

            Text {
                text: diffModal.targetFile
                color: theme.amber
                bold: true
            }

            Text {
                text: " · [Esc/F3] close"
                color: theme.muted
            }
        }

        // Subheader stats
        Row {
            width: parent.width
            height: 1
            spacing: 1

            Text {
                text: "@@ -72,6 +72,8 @@ "
                color: theme.cyan
                bold: true
            }
            Text {
                text: "+3 additions"
                color: theme.green
            }
            Text {
                text: "·"
                color: theme.muted
            }
            Text {
                text: "-1 deletion"
                color: theme.rose
            }
        }

        // Column Titles (Before / After Side-by-Side)
        Row {
            width: parent.width
            height: 1
            spacing: 0

            Text {
                width: 36
                text: "  Original (HEAD)"
                color: theme.muted
                bold: true
            }
            Text {
                text: "  "
                color: theme.muted
            }
            Text {
                text: "Modified (proposed patch)"
                color: theme.muted
                bold: true
            }
        }

        // Code Diff Lines Side-by-Side
        Rectangle {
            width: parent.width
            height: 1
            color: "transparent"
            Row {
                width: parent.width
                height: 1
                spacing: 0
                Text { width: 36; text: " 72: void StreamRenderer::render() {"; color: theme.slate }
                Text { text: "  "; color: theme.muted }
                Text { text: " 72: void StreamRenderer::render() {"; color: theme.slate }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#1e1e2e"
            Row {
                width: parent.width
                height: 1
                spacing: 0
                Text { width: 36; text: " 73:   const int h = buffer.height();"; color: theme.slate }
                Text { text: "  "; color: theme.muted }
                Text { text: " 73:   const int h = buffer.height();"; color: theme.slate }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#2a1215"
            Row {
                width: parent.width
                height: 1
                spacing: 0
                Text { width: 36; text: "-74:   device.write(buffer.data());"; color: theme.rose }
                Text { text: "  "; color: theme.muted }
                Text { text: "    (replaced with bounds check)"; color: theme.muted }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#0d2818"
            Row {
                width: parent.width
                height: 1
                spacing: 0
                Text { width: 36; text: "                                    "; color: theme.muted }
                Text { text: "  "; color: theme.muted }
                Text { text: "+74:   const int safeHeight = clamp(h);"; color: theme.green }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#0d2818"
            Row {
                width: parent.width
                height: 1
                spacing: 0
                Text { width: 36; text: "                                    "; color: theme.muted }
                Text { text: "  "; color: theme.muted }
                Text { text: "+75:   markDirty(safeHeight);"; color: theme.green }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#0d2818"
            Row {
                width: parent.width
                height: 1
                spacing: 0
                Text { width: 36; text: "                                    "; color: theme.muted }
                Text { text: "  "; color: theme.muted }
                Text { text: "+76:   device.write(buffer.slice(...));"; color: theme.green }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "transparent"
            Row {
                width: parent.width
                height: 1
                spacing: 0
                Text { width: 36; text: " 77: }"; color: theme.slate }
                Text { text: "  "; color: theme.muted }
                Text { text: " 77: }"; color: theme.slate }
            }
        }
    }

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_F3) {
            close()
            event.accepted = true
        }
    }
}
