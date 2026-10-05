pragma ComponentBehavior: Bound

import QtQml
import Qtui

Rectangle {
    id: root

    required property var currentEntries
    required property int currentIndex
    required property string filterText
    required property bool isFilterActive

    signal itemClicked(int index)

    color: Theme.bgColumn
    border.style: "single"
    border.color: Theme.borderActive

    // Column Header
    Rectangle {
        id: colHeader
        anchors.top: parent.top
        anchors.topMargin: 1
        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.right: parent.right
        anchors.rightMargin: 1
        height: 1
        color: Theme.bgPanel

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 1
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1

            Text {
                text: "CURRENT"
                color: Theme.accentBlue
                bold: true
            }

            Text {
                text: "(" + (root.currentEntries ? root.currentEntries.length : 0) + ")"
                color: Theme.textMuted
            }
        }

        // Active Filter indicator
        Row {
            visible: root.filterText !== "" || root.isFilterActive
            anchors.right: parent.right
            anchors.rightMargin: 1
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1

            Text {
                text: "filter:"
                color: Theme.accentPink
                bold: true
            }

            Text {
                text: root.filterText === "" ? "_" : root.filterText
                color: Theme.textHighlight
                bold: true
            }
        }
    }

    ListView {
        id: entryList
        anchors.top: colHeader.bottom
        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.right: parent.right
        anchors.rightMargin: 1
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        clip: true
        model: root.currentEntries
        currentIndex: root.currentIndex

        delegate: Rectangle {
            id: entryDelegate

            required property int index
            required property var modelData

            width: entryList.width
            height: 1

            readonly property bool isCurrent: entryDelegate.index === root.currentIndex
            readonly property bool isMarked: entryDelegate.modelData.isMarked || false

            color: {
                if (entryDelegate.isCurrent && entryDelegate.isMarked) return Theme.bgSelected;
                if (entryDelegate.isCurrent) return Theme.bgCursor;
                if (entryDelegate.isMarked) return Theme.bgMarked;
                return "transparent";
            }

            // Left side: Cursor, Mark checkbox, Icon, File Name
            Row {
                x: 1
                anchors.verticalCenter: parent.verticalCenter
                spacing: 1

                // Cursor pointer
                Text {
                    text: entryDelegate.isCurrent ? "▸" : " "
                    color: Theme.accentYellow
                    bold: true
                }

                // Selection checkmark
                Text {
                    text: entryDelegate.isMarked ? "[✓]" : "   "
                    color: Theme.accentPeach
                    bold: true
                }

                // File type icon
                Text {
                    text: entryDelegate.modelData.icon || "📄"
                    color: entryDelegate.modelData.iconColor || Theme.textPrimary
                }

                // File / Directory Name
                Text {
                    text: entryDelegate.modelData.name || ""
                    color: {
                        if (entryDelegate.isCurrent) return Theme.textHighlight;
                        if (entryDelegate.isMarked) return Theme.accentPeach;
                        if (entryDelegate.modelData.isDir) return Theme.accentBlue;
                        return Theme.textPrimary;
                    }
                    bold: entryDelegate.isCurrent || entryDelegate.modelData.isDir
                }
            }

            // Right side: File size or item count
            Text {
                anchors.right: parent.right
                anchors.rightMargin: 1
                anchors.verticalCenter: parent.verticalCenter
                text: entryDelegate.modelData.sizeFormatted || ""
                color: entryDelegate.isCurrent ? Theme.textSecondary : Theme.textDim
            }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: !root.currentEntries || root.currentEntries.length === 0
        text: root.filterText !== "" ? "No matching files" : "(Empty directory)"
        color: Theme.textDim
    }
}
