pragma ComponentBehavior: Bound

import QtQml
import Qtui

Rectangle {
    id: root

    required property var previewData
    required property int currentIndex
    required property int totalCount
    required property int markedCount
    required property string sortField
    required property bool sortAscending
    required property string statusMessage

    height: 2
    color: Theme.bgHeader
    border.style: "none"

    // Line 1: Item details and position stats
    Row {
        x: 1
        y: 0
        width: parent.width - 2
        spacing: 2

        Text {
            text: {
                if (root.previewData && root.previewData.permissions) {
                    return root.previewData.permissions + "  " + (root.previewData.owner || "");
                }
                return "-";
            }
            color: Theme.textMuted
        }

        Text {
            visible: root.statusMessage !== ""
            text: " " + root.statusMessage + " "
            color: Theme.accentYellow
            bold: true
        }

        // Right side info: Marked items, Sort mode, Index/percentage
        Row {
            anchors.right: parent.right
            spacing: 2

            Text {
                visible: root.markedCount > 0
                text: "[" + root.markedCount + " marked]"
                color: Theme.accentPeach
                bold: true
            }

            Text {
                text: "Sort: " + root.sortField + (root.sortAscending ? " ▲" : " ▼")
                color: Theme.textSecondary
            }

            Text {
                text: {
                    if (root.totalCount === 0) return "0/0 (0%)";
                    const pos = (root.currentIndex + 1);
                    const pct = Math.round((pos / root.totalCount) * 100);
                    return pos + "/" + root.totalCount + " (" + pct + "%)";
                }
                color: Theme.accentCyan
                bold: true
            }
        }
    }

    // Line 2: Quick Keybinding Cheatsheet Bar
    Row {
        x: 1
        y: 1
        width: parent.width - 2
        spacing: 1

        Text {
            text: "[?]Help  [h/l]Nav  [j/k]Move  [Space]Mark  [/]Filter  [a]New  [r]Rename  [d]Del  [y/p]Copy/Paste  [.]Dotfiles  [s]Sort  [q]Quit"
            color: Theme.textDim
        }
    }
}
