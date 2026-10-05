pragma ComponentBehavior: Bound

import QtQml
import Qtui

Rectangle {
    id: root

    required property var previewData
    property var backend: null

    onWidthChanged: {
        if (root.backend && root.width > 0) {
            root.backend.previewWidth = root.width;
        }
    }

    color: Theme.bgColumnDim
    border.style: "single"
    border.color: Theme.borderNormal

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
                text: "PREVIEW"
                color: Theme.accentGreen
                bold: true
            }

            Text {
                visible: Boolean(root.previewData && root.previewData.name)
                text: "· " + (root.previewData && root.previewData.name ? root.previewData.name : "")
                color: Theme.textSecondary
            }
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 1
            anchors.verticalCenter: parent.verticalCenter
            text: {
                if (!root.previewData) return "";
                if (root.previewData.type === "text") return (root.previewData.lineCount || 0) + " lines";
                if (root.previewData.type === "directory") return (root.previewData.itemCount || 0) + " items";
                return root.previewData.sizeFormatted || "";
            }
            color: Theme.textDim
        }
    }

    // 1. Text File Preview (Word-wrapped with Paragraph)
    Item {
        id: textPreviewPane
        visible: Boolean(root.previewData && root.previewData.type === "text")
        anchors.top: colHeader.bottom
        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.right: parent.right
        anchors.rightMargin: 1
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        clip: true

        ScrollView {
            id: textScrollView
            anchors.fill: parent
            clip: true

            Paragraph {
                id: textParagraph
                width: textScrollView.width
                wrapMode: false
                textFormat: Paragraph.PlainText
                color: Theme.textPrimary
                text: root.previewData ? (root.previewData.formattedText || root.previewData.rawText || (root.previewData.textLines ? root.previewData.textLines.join("\n") : "")) : ""
            }
        }
    }

    // 2. Directory Child Items Preview
    Item {
        id: dirPreviewPane
        visible: Boolean(root.previewData && root.previewData.type === "directory")
        anchors.top: colHeader.bottom
        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.right: parent.right
        anchors.rightMargin: 1
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        clip: true

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1

            // Directory Info Card
            Rectangle {
                width: parent.width
                height: 3
                color: Theme.bgPanel
                border.style: "single"
                border.color: Theme.borderNormal

                Column {
                    x: 1
                    y: 0
                    spacing: 0

                    Row {
                        spacing: 1
                        Text { text: "📁 " + (root.previewData ? root.previewData.name : ""); color: Theme.accentBlue; bold: true }
                        Text { text: "(" + (root.previewData ? root.previewData.itemCount : 0) + " items)"; color: Theme.textSecondary }
                    }

                    Row {
                        spacing: 2
                        Text { text: "Modified: " + (root.previewData ? root.previewData.modifiedFormatted : ""); color: Theme.textDim }
                        Text { text: "Perms: " + (root.previewData ? root.previewData.permissions : ""); color: Theme.textDim }
                    }
                }
            }

            // Child Entries List
            ListView {
                id: dirChildList
                width: parent.width
                height: Math.max(1, parent.height - 4)
                clip: true
                model: root.previewData ? root.previewData.dirChildren : []

                delegate: Rectangle {
                    id: dirChildDelegate

                    required property int index
                    required property var modelData

                    width: dirChildList.width
                    height: 1
                    color: "transparent"

                    Row {
                        x: 1
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 1

                        Text {
                            text: dirChildDelegate.modelData.icon || "📄"
                            color: dirChildDelegate.modelData.iconColor || Theme.textPrimary
                        }

                        Text {
                            text: dirChildDelegate.modelData.name || ""
                            color: dirChildDelegate.modelData.isDir ? Theme.accentBlue : Theme.textPrimary
                        }
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 1
                        anchors.verticalCenter: parent.verticalCenter
                        text: dirChildDelegate.modelData.sizeFormatted || ""
                        color: Theme.textDim
                    }
                }
            }
        }
    }

    // 3. Binary / Image / Archive Metadata Card
    Item {
        id: binaryPreviewPane
        visible: Boolean(root.previewData && (root.previewData.type === "binary" || root.previewData.type === "image"))
        anchors.top: colHeader.bottom
        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.right: parent.right
        anchors.rightMargin: 1
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1

        Column {
            anchors.fill: parent
            anchors.margins: 0
            spacing: 1

            Rectangle {
                width: parent.width
                height: 6
                color: Theme.bgPanel
                border.style: "single"
                border.color: Theme.borderNormal

                Column {
                    x: 1
                    y: 0
                    spacing: 0

                    Row {
                        spacing: 1
                        Text { text: root.previewData && root.previewData.type === "image" ? "🖼️ Image Asset" : "📦 Binary / Archive"; color: Theme.accentPink; bold: true }
                    }

                    Text { text: "File: " + (root.previewData ? root.previewData.name : ""); color: Theme.textPrimary; bold: true }
                    Text { text: "MIME: " + (root.previewData ? root.previewData.mimeType : ""); color: Theme.accentCyan }
                    Text { text: "Size: " + (root.previewData ? root.previewData.sizeFormatted : ""); color: Theme.textSecondary }
                    Text { text: "Perms: " + (root.previewData ? root.previewData.permissions : "") + "  Owner: " + (root.previewData ? root.previewData.owner : ""); color: Theme.textDim }
                }
            }

            Rectangle {
                width: parent.width
                height: 4
                color: Theme.bgColumn
                border.style: "single"
                border.color: Theme.borderNormal

                Text {
                    anchors.centerIn: parent
                    text: root.previewData && root.previewData.type === "image" ? "[ Image Content Preview ]" : "[ Binary Data Block ]"
                    color: Theme.textMuted
                }
            }
        }
    }

    // 4. Empty state
    Text {
        anchors.centerIn: parent
        visible: Boolean(!root.previewData || root.previewData.type === "empty" || (!root.previewData.type))
        text: "(No file selected)"
        color: Theme.textDim
    }
}
