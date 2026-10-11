pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Controls
import Qamui.Image
import BrandingLogosDemo

Terminal {
    id: root
    title: "Qamui Branding & Terminal Logo Showcase"

    property int initialIndex: 0
    property int selectedIndex: initialIndex
    property bool isDarkTheme: true
    property bool smoothRendering: true
    property int renderModeIndex: 0

    readonly property var renderModes: [
        { mode: Image.Auto, name: "Auto" },
        { mode: Image.HalfBlock, name: "HalfBlock" },
        { mode: Image.Braille, name: "Braille" },
        { mode: Image.ASCII, name: "ASCII" },
        { mode: Image.Sixel, name: "Sixel" },
        { mode: Image.Kitty, name: "Kitty" }
    ]
    readonly property int currentRenderMode: renderModes[renderModeIndex].mode
    readonly property string currentRenderModeName: renderModes[renderModeIndex].name

    // Palette tokens
    color: isDarkTheme ? "#0B1020" : "#F6F7F9"
    readonly property string fgColor: isDarkTheme ? "#F6F7F9" : "#1A1D26"
    readonly property string fgDim: isDarkTheme ? "#7E8B9B" : "#6E7781"
    readonly property string accentColor: isDarkTheme ? "#00FF66" : "#00A651"
    readonly property string panelBg: isDarkTheme ? "#10162A" : "#FFFFFF"
    readonly property string panelBorder: isDarkTheme ? "#2D3748" : "#D0D7DE"
    readonly property string highlightBg: isDarkTheme ? "#1E2A4A" : "#E2E8F0"

    readonly property var currentItem: BrandingCatalog.get(selectedIndex)
    readonly property bool hasItems: BrandingCatalog.count > 0

    // Terminal graphic capabilities
    readonly property bool hasKitty: capabilities.kittyGraphics
    readonly property bool hasSixel: capabilities.sixelGraphics
    readonly property string detectedProtocol: hasKitty ? "Kitty" : (hasSixel ? "Sixel" : "Unicode")

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_K) {
            if (root.selectedIndex > 0)
                root.selectedIndex--
            else if (BrandingCatalog.count > 0)
                root.selectedIndex = BrandingCatalog.count - 1
            event.accepted = true
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_J) {
            if (root.selectedIndex < BrandingCatalog.count - 1)
                root.selectedIndex++
            else
                root.selectedIndex = 0
            event.accepted = true
        } else if (event.key === Qt.Key_M) {
            root.renderModeIndex = (root.renderModeIndex + 1) % root.renderModes.length
            event.accepted = true
        } else if (event.key === Qt.Key_T) {
            root.isDarkTheme = !root.isDarkTheme
            event.accepted = true
        } else if (event.key === Qt.Key_S) {
            root.smoothRendering = !root.smoothRendering
            event.accepted = true
        } else if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
            const target = event.key - Qt.Key_1
            if (target < BrandingCatalog.count)
                root.selectedIndex = target
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Top Header
        Row {
            width: parent.width
            height: 1
            spacing: 2

            Text {
                text: "Qamui"
                color: root.accentColor
                bold: true
            }

            Text {
                text: "│ Branding & Logo Terminal Showcase"
                color: root.fgColor
                bold: true
            }

            Item {
                width: Math.max(1, parent.width - 76)
                height: 1
            }

            Text {
                text: "[1-9] Jump  [↑/↓] Select  [M] Mode  [T] Theme  [Q] Quit"
                color: root.fgDim
            }
        }

        // Main Center Area: Left Sidebar (List) + Right Canvas (Logo viewer)
        Row {
            width: parent.width
            height: Math.max(10, parent.height - 4)
            spacing: 1

            // Left Sidebar: Assets list
            Panel {
                id: sidebar
                width: 30
                height: parent.height
                title: "Assets (" + BrandingCatalog.count + ")"
                color: root.panelBg
                border.style: "single"
                border.color: root.panelBorder
                titleColor: root.accentColor

                ScrollView {
                    anchors.fill: parent
                    anchors.leftMargin: 1
                    anchors.rightMargin: 1
                    anchors.topMargin: 1
                    anchors.bottomMargin: 1

                    Column {
                        width: parent.width
                        spacing: 0

                        Repeater {
                            model: BrandingCatalog.count
                            delegate: Rectangle {
                                id: itemRow
                                required property int index
                                readonly property var itemData: BrandingCatalog.get(index)
                                readonly property bool isSelected: index === root.selectedIndex

                                width: parent.width
                                height: 1
                                color: isSelected ? root.highlightBg : "transparent"

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.selectedIndex = itemRow.index
                                }

                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: 1
                                    spacing: 1

                                    Text {
                                        text: itemRow.isSelected ? "▶" : (itemRow.index < 9 ? "" + (itemRow.index + 1) + " " : "  ")
                                        color: itemRow.isSelected ? root.accentColor : root.fgDim
                                        bold: itemRow.isSelected
                                    }

                                    Text {
                                        text: itemRow.itemData.title
                                        color: itemRow.isSelected ? root.accentColor : root.fgColor
                                        bold: itemRow.isSelected
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Right Canvas: Logo Display Area
            Panel {
                id: canvasPanel
                width: parent.width - sidebar.width - 1
                height: parent.height
                title: root.hasItems ? (root.currentItem.title + "  [" + root.currentItem.category + "]") : "No Assets"
                color: root.panelBg
                border.style: "single"
                border.color: root.panelBorder
                titleColor: root.accentColor

                // Description bar
                Text {
                    x: 2
                    y: 1
                    width: parent.width - 4
                    height: 1
                    text: root.hasItems ? root.currentItem.description : ""
                    color: root.fgDim
                }

                // Center viewing viewport
                Item {
                    x: 1
                    y: 2
                    width: parent.width - 2
                    height: parent.height - 3

                    // Image view for vector SVGs and raster PNGs
                    Image {
                        id: imageViewer
                        visible: root.hasItems && root.currentItem.type === "image"
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 2, 64)
                        height: Math.min(parent.height - 2, 20)
                        source: (root.hasItems && root.currentItem.type === "image") ? root.currentItem.source : ""
                        fillMode: Image.PreserveAspectFit
                        renderMode: root.currentRenderMode
                        smooth: root.smoothRendering
                    }

                    // Direct text view for pre-rendered ANSI TrueColor and Braille cells
                    Text {
                        id: textViewer
                        visible: root.hasItems && root.currentItem.type === "ansi"
                        anchors.centerIn: parent
                        text: (root.hasItems && root.currentItem.type === "ansi") ? root.currentItem.text : ""
                        color: root.fgColor
                    }
                }
            }
        }

        // Bottom Status Bar
        Row {
            width: parent.width
            height: 1
            spacing: 2

            Text {
                text: "Theme: " + (root.isDarkTheme ? "Dark" : "Light") + " [T]"
                color: root.isDarkTheme ? "#00FF66" : "#00A651"
                bold: true
            }

            Text {
                text: "│"
                color: root.panelBorder
            }

            Text {
                visible: root.hasItems && root.currentItem.type === "image"
                text: "Mode: " + root.currentRenderModeName + " [M]"
                color: root.accentColor
            }

            Text {
                visible: root.hasItems && root.currentItem.type === "ansi"
                text: "Format: Native Cells"
                color: root.accentColor
            }

            Text {
                text: "│"
                color: root.panelBorder
            }

            Text {
                text: root.hasItems ? ("Size: " + root.currentItem.dimensions) : ""
                color: root.fgDim
            }

            Text {
                text: "│"
                color: root.panelBorder
            }

            Text {
                text: root.hasItems ? ("File: " + root.currentItem.id) : ""
                color: root.fgColor
            }

            Text {
                text: "│"
                color: root.panelBorder
            }

            Text {
                text: "Protocol: " + root.detectedProtocol
                color: root.fgDim
            }
        }
    }
}
