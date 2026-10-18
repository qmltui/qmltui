pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// QmlTui Gallery: Master-Detail Component Explorer & Live Inspector.
// Left pane: categorized component sidebar. Right pane: live interactive stage.
// Arrow keys (or hjkl) navigate the sidebar, Enter/Space or a mouse click
// activates the live interactive component inspector in the right stage, F2
// toggles full-width stage, and Escape returns focus to the sidebar (or quits
// when already in the sidebar).
Terminal {
    id: root
    title: showingDemo && current ? "QmlTui Gallery · " + current.file : "QmlTui Gallery"
    color: inlineMode ? "default" : (lightTheme ? "#ffffff" : "#0d1117")

    property bool lightTheme: false
    readonly property string brandPrimary: lightTheme ? "#00a651" : "#2cde85"
    readonly property string brandAccent: lightTheme ? "#008f45" : "#00ff66"
    readonly property string panelColor: "default"
    readonly property string tileColor: lightTheme ? "#f6f8fa" : "#161b22"
    readonly property string tileCurrentColor: lightTheme ? "#e6f7ef" : "#12261e"
    readonly property string lineColor: lightTheme ? "#d0d7de" : "#30363d"
    readonly property string textColor: lightTheme ? "#1a1d26" : "#c9d1d9"
    readonly property string mutedColor: lightTheme ? "#57606a" : "#8b949e"

    // Navigation & master-detail state
    property int currentIndex: 0
    property int openIndex: -1
    property int sidebarScroll: 0
    property bool fullscreenStage: false
    readonly property bool showingDemo: openIndex >= 0
    readonly property var current: showingDemo ? catalog[openIndex] : (catalog.length > 0 ? catalog[currentIndex] : null)
    readonly property bool isInlineExample: showingDemo && current !== null && current.file === "InlineMode.qml"
    mode: isInlineExample ? Terminal.Inline : Terminal.AltScreen
    readonly property bool inlineMode: mode === Terminal.Inline

    readonly property string standaloneFile: typeof qmltuiStandaloneExample === "string" ? qmltuiStandaloneExample : ""
    readonly property bool standalone: standaloneFile !== ""
    readonly property string initialPageFile: typeof qmltuiGalleryPage === "string" ? qmltuiGalleryPage : ""
    readonly property var availableFiles: typeof qmltuiAvailableExamples !== "undefined" && qmltuiAvailableExamples !== null && typeof qmltuiAvailableExamples.length === "number" ? qmltuiAvailableExamples : null

    readonly property int sidebarWidth: (fullscreenStage || standalone) ? 0 : 26
    readonly property int sidebarVisibleRows: Math.max(1, bodyArea.height - 2)

    // Quick-jump grid inside the stage overview (when !showingDemo)
    readonly property int cardWidth: 22
    readonly property int cardHeight: 3
    readonly property int cardGap: 1
    readonly property int gridCols: Math.max(1, Math.floor((overviewGrid.width + cardGap) / (cardWidth + cardGap)))
    readonly property int gridVisRows: Math.max(1, Math.floor((overviewGrid.height + cardGap) / (cardHeight + cardGap)))
    readonly property int gridTotalRows: Math.ceil(catalog.length / gridCols)
    property int gridScrollRow: 0

    readonly property var catalog: availableFiles ? allEntries.filter(entry => availableFiles.indexOf(entry.file) >= 0) : allEntries
    readonly property var allEntries: [
        {
            name: "Button",
            badge: "Controls",
            accent: "#2cde85",
            preview: "[ Primary ]  [ Filled ]  [ 💾 Save ]",
            blurb: "Push, checkable, hover-reactive and custom-chrome buttons",
            file: "Button.qml"
        },
        {
            name: "CheckBox",
            badge: "Controls",
            accent: "#7ee0ff",
            preview: "[x] TLS 1.3  [x] Audit  [ ] Trace",
            blurb: "Boolean policy toggles with live flag synthesis",
            file: "CheckBox.qml"
        },
        {
            name: "RadioButton",
            badge: "Controls",
            accent: "#f9c74f",
            preview: "(•) Release -O2   ( ) Ultra -O3",
            blurb: "Mutually exclusive option groups via ButtonGroup",
            file: "RadioButton.qml"
        },
        {
            name: "Switch",
            badge: "Controls",
            accent: "#ff9f6e",
            preview: "[ ON] Primary DB  [OFF] Workers",
            blurb: "Binary power switches for subsystem control",
            file: "Switch.qml"
        },
        {
            name: "Slider",
            badge: "Controls",
            accent: "#c084fc",
            preview: "──────●────────── [ 42% ]",
            blurb: "Continuous or stepped value track with keyboard & drag",
            file: "Slider.qml"
        },
        {
            name: "ProgressBar",
            badge: "Controls",
            accent: "#38bdf8",
            preview: "▰▰▰▰▰▰▱▱▱▱  58%  [████░░]",
            blurb: "Block, segmented, ASCII, gradient and threshold meters",
            file: "ProgressBar.qml"
        },
        {
            name: "BusyIndicator",
            badge: "Controls",
            accent: "#f472b6",
            preview: "⠋ building   ◐ syncing   [ =  ]",
            blurb: "Configurable multi-frame terminal activity spinners",
            file: "BusyIndicator.qml"
        },
        {
            name: "TabBar",
            badge: "Controls",
            accent: "#fbbf24",
            preview: "│ Overview │ Network │ Storage │",
            blurb: "Horizontal tab strip driving a multi-page StackLayout",
            file: "TabBar.qml"
        },
        {
            name: "Panel",
            badge: "Controls",
            accent: "#94a3b8",
            preview: "╭─ Title [BADGE] ────── info ─╮",
            blurb: "Titled container frame with header & footer badges",
            file: "Panel.qml"
        },
        {
            name: "Dialog",
            badge: "Controls",
            accent: "#f87171",
            preview: "╔═ Confirm ═╗  [ OK ] [Cancel]",
            blurb: "Modal confirmation dialog with contextual Escape",
            file: "Dialog.qml"
        },
        {
            name: "Popup",
            badge: "Controls",
            accent: "#a3e635",
            preview: "╔══ Command Overlay ══╗",
            blurb: "Modal overlay surface with backdrop dimming & focus trap",
            file: "Popup.qml"
        },
        {
            name: "Toast",
            badge: "Controls",
            accent: "#34d399",
            preview: "┌ ✓ Deployment succeeded ───┐",
            blurb: "Auto-dismissing corner notification queue",
            file: "Toast.qml"
        },
        {
            name: "ToolTip",
            badge: "Controls",
            accent: "#fcd34d",
            preview: "┌ Hint: Ctrl+D deploys ┐",
            blurb: "Attached contextual help overlay with delay & timeout",
            file: "ToolTip.qml"
        },
        {
            name: "Sparkline",
            badge: "Controls",
            accent: "#60a5fa",
            preview: "▁▂▄▆█▆▄▃▅▇█▆▄▂  CPU 64%",
            blurb: "Compact Braille and block-element telemetry charts",
            file: "Sparkline.qml"
        },
        {
            name: "StackView",
            badge: "Controls",
            accent: "#e879f9",
            preview: "Step 1 ▸ Step 2 ▸ Confirm (depth 3)",
            blurb: "Push/pop navigation stack for multi-step workflows",
            file: "StackView.qml"
        },
        {
            name: "Rectangle",
            badge: "Layout",
            accent: "#ec4899",
            preview: "╭─ Rounded ─╮ ╔═ Double ═╗ ▓▓▒▒░░",
            blurb: "Filled surfaces, 4 border styles, and linear gradients",
            file: "Rectangle.qml"
        },
        {
            name: "Layouts",
            badge: "Layout",
            accent: "#14b8a6",
            preview: "┌── Row / Column / GridLayout ──┐",
            blurb: "Stretch factors, cell spans and dynamic reflow",
            file: "Layouts.qml"
        },
        {
            name: "Text",
            badge: "Text",
            accent: "#e2e8f0",
            preview: "Bold · Italic · Underline · Gradient",
            blurb: "Cell typography, word wrapping, alignment & gradients",
            file: "Text.qml"
        },
        {
            name: "RichText",
            badge: "Text",
            accent: "#fda4af",
            preview: "Spans · <b>HTML</b> · \\x1b[32mANSI",
            blurb: "Declarative spans, HTML-subset tags & SGR escape parsing",
            file: "RichText.qml"
        },
        {
            name: "TextInput",
            badge: "Text",
            accent: "#67e8f9",
            preview: "┌ quinn█────────────────────┐",
            blurb: "Single-line editor with validation, echo modes & undo",
            file: "TextInput.qml"
        },
        {
            name: "TextArea",
            badge: "Text",
            accent: "#818cf8",
            preview: "1 │ import QmlTui\\n2 │ Terminal { }",
            blurb: "Multi-line text editor with syntax highlight layers",
            file: "TextArea.qml"
        },
        {
            name: "ListView",
            badge: "Views",
            accent: "#2dd4bf",
            preview: "▸ #1 Build Core    [DONE]  1.2s",
            blurb: "Virtualized list view with delegate recycling",
            file: "ListView.qml"
        },
        {
            name: "GridView",
            badge: "Views",
            accent: "#f43f5e",
            preview: "┌Tile 1┐ ┌Tile 2┐ ┌Tile 3┐",
            blurb: "Virtualized 2D cell grid with arrow key navigation",
            file: "GridView.qml"
        },
        {
            name: "Image",
            badge: "Image",
            accent: "#f59e0b",
            preview: "🐼 Braille · HalfBlock · ASCII · Auto",
            blurb: "Bitmaps rendered via Braille, HalfBlock, Sixel or Kitty",
            file: "Image.qml"
        },
        {
            name: "AnimatedSprite",
            badge: "Image",
            accent: "#ec4899",
            preview: "🎞 Frame 1/4 · 256x64 sprite sheet",
            blurb: "Frames out of one sheet with frame slicing & reverse",
            file: "AnimatedSprite.qml"
        },
        {
            name: "TableView",
            badge: "Views",
            accent: "#fb923c",
            preview: "PID │ NAME     │ CPU% │ STATE",
            blurb: "Multi-column data table with custom cell formatting",
            file: "TableView.qml"
        },
        {
            name: "TreeView",
            badge: "Views",
            accent: "#84cc16",
            preview: "▾ src/\\n  ▾ QmlTui/\\n    ▸ Controls/",
            blurb: "Expandable hierarchical tree view with depth tracking",
            file: "TreeView.qml"
        },
        {
            name: "ScrollView",
            badge: "Views",
            accent: "#38bdf8",
            preview: "│ Viewport content      ▐│",
            blurb: "Scrollable viewport pane with proportional scrollbar",
            file: "ScrollView.qml"
        },
        {
            name: "Canvas",
            badge: "Canvas",
            accent: "#10b981",
            preview: "╭─ Braille 2x4 & HalfBlock 1x2 ─╮",
            blurb: "Sub-cell pixel & vector canvas (Line, Circle, Polygon)",
            file: "Canvas.qml"
        },
        {
            name: "Repeater",
            badge: "Layout",
            accent: "#eab308",
            preview: "[Node 1] [Node 2] [Node 3] (+Add)",
            blurb: "Model-driven item instantiation inside layouts",
            file: "Repeater.qml"
        },
        {
            name: "Loader",
            badge: "Layout",
            accent: "#6366f1",
            preview: "Loader { sourceComponent: card }",
            blurb: "Lazy loading and dynamic swapping of QML components",
            file: "Loader.qml"
        },
        {
            name: "MouseArea",
            badge: "Input",
            accent: "#06b6d4",
            preview: "┼ (x: 24, y: 8) · hover & wheel",
            blurb: "Pointer clicks, drag tracking, hover & scroll wheel",
            file: "MouseArea.qml"
        },
        {
            name: "Shortcut",
            badge: "Input",
            accent: "#8b5cf6",
            preview: "[1] Build  [2] Test  [3] Deploy",
            blurb: "Declarative key sequence bindings and action routing",
            file: "Shortcut.qml"
        },
        {
            name: "States",
            badge: "Motion",
            accent: "#d946ef",
            preview: "nominal ──► degraded ──► alert",
            blurb: "Named property configurations with animated Transitions",
            file: "States.qml"
        },
        {
            name: "Animation",
            badge: "Motion",
            accent: "#f43f5e",
            preview: "Linear · OutQuad · OutBounce ───●",
            blurb: "Number, Color, Sequential and Parallel timeline animations",
            file: "Animation.qml"
        },
        {
            name: "Behavior",
            badge: "Motion",
            accent: "#22d3ee",
            preview: "Behavior on x { NumberAnimation }",
            blurb: "Implicit property transition animations on value change",
            file: "Behavior.qml"
        },
        {
            name: "InlineMode",
            badge: "CLI",
            accent: "#38bdf8",
            preview: "❯ hello from inline  (printed: 1)",
            blurb: "Inline mode with persistent Scrollback history",
            file: "InlineMode.qml"
        }
    ]

    function ensureVisible() {
        if (currentIndex < sidebarScroll)
            sidebarScroll = currentIndex;
        else if (currentIndex >= sidebarScroll + sidebarVisibleRows)
            sidebarScroll = currentIndex - sidebarVisibleRows + 1;
        sidebarScroll = Math.max(0, Math.min(sidebarScroll, Math.max(0, catalog.length - sidebarVisibleRows)));

        const gRow = Math.floor(currentIndex / gridCols);
        if (gRow < gridScrollRow)
            gridScrollRow = gRow;
        else if (gRow >= gridScrollRow + gridVisRows)
            gridScrollRow = gRow - gridVisRows + 1;
        gridScrollRow = Math.max(0, Math.min(gridScrollRow, Math.max(0, gridTotalRows - gridVisRows)));
    }

    function move(delta) {
        const next = Math.max(0, Math.min(catalog.length - 1, currentIndex + delta));
        currentIndex = next;
    }

    function openCurrent() {
        openIndex = currentIndex;
    }

    function focusExample(item) {
        let fallback = null;
        const marked = (function visit(node) {
                for (let i = 0; i < node.children.length; ++i) {
                    const child = node.children[i];
                    if (child.focus)
                        return child;
                    if (!fallback && child.focusable && child.visible && child.enabled)
                        fallback = child;
                    const found = visit(child);
                    if (found)
                        return found;
                }
                return null;
            })(item);
        const target = marked || fallback;
        if (target)
            target.focus = true;
    }

    function closeDemo() {
        if (standalone) {
            Qt.quit();
            return;
        }
        openIndex = -1;
        root.focus = true;
    }

    Component.onCompleted: {
        const targetFile = standalone ? standaloneFile : initialPageFile;
        if (!targetFile)
            return;
        const index = catalog.findIndex(entry => entry.file === targetFile);
        if (index >= 0) {
            currentIndex = index;
            openCurrent();
        }
    }

    onCurrentIndexChanged: ensureVisible()
    onSidebarVisibleRowsChanged: ensureVisible()
    onGridColsChanged: ensureVisible()
    onGridVisRowsChanged: ensureVisible()

    Keys.onPressed: event => {
        if (event.key === Qt.Key_F2 && !standalone) {
            fullscreenStage = !fullscreenStage;
            event.accepted = true;
            return;
        }
        if (showingDemo) {
            if (event.key === Qt.Key_Escape || (event.key === Qt.Key_Backspace && !standalone)) {
                closeDemo();
                event.accepted = true;
            } else if (event.key === Qt.Key_Q) {
                Qt.quit();
                event.accepted = true;
            }
            return;
        }
        switch (event.key) {
        case Qt.Key_Left:
        case Qt.Key_Up:
        case Qt.Key_H:
        case Qt.Key_K:
            move(-1);
            break;
        case Qt.Key_Right:
        case Qt.Key_Down:
        case Qt.Key_L:
        case Qt.Key_J:
            move(1);
            break;
        case Qt.Key_PageUp:
            move(-sidebarVisibleRows);
            break;
        case Qt.Key_PageDown:
            move(sidebarVisibleRows);
            break;
        case Qt.Key_Home:
            currentIndex = 0;
            break;
        case Qt.Key_End:
            currentIndex = catalog.length - 1;
            break;
        case Qt.Key_Return:
        case Qt.Key_Enter:
        case Qt.Key_Space:
            openCurrent();
            break;
        case Qt.Key_T:
            lightTheme = !lightTheme;
            break;
        case Qt.Key_Q:
        case Qt.Key_Escape:
            Qt.quit();
            break;
        default:
            return;
        }
        event.accepted = true;
    }

    // Top Master Header Bar
    Rectangle {
        id: header
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 2
        visible: !root.inlineMode && !root.standalone
        color: root.tileColor

        Text {
            id: brandTitle
            x: 2
            y: 0
            text: "QmlTui Gallery"
            bold: true
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop {
                    position: 0.0
                    color: root.brandAccent
                }
                GradientStop {
                    position: 0.55
                    color: root.brandPrimary
                }
                GradientStop {
                    position: 1.0
                    color: "#58a6ff"
                }
            }
        }

        Text {
            anchors {
                left: brandTitle.right
                leftMargin: 2
                right: parent.right
                rightMargin: 2
            }
            x: 18
            y: 0
            width: Math.max(0, parent.width - 30)
            clip: true
            visible: root.current !== null
            spans: [
                {
                    text: " · ",
                    color: root.mutedColor
                },
                {
                    text: root.current ? root.current.name : "",
                    color: root.current ? root.current.accent : root.textColor,
                    bold: true
                },
                {
                    text: " [" + (root.current ? root.current.badge : "") + "] ",
                    color: root.brandPrimary
                },
                {
                    text: root.showingDemo ? " · examples/" + (root.current ? root.current.file : "") + "  ◂ Esc back to grid" : "— " + (root.current ? root.current.blurb : ""),
                    color: root.mutedColor
                }
            ]
        }

        Text {
            anchors {
                right: parent.right
                rightMargin: 2
            }
            y: 0
            text: (root.currentIndex + 1) + " / " + root.catalog.length
            color: root.brandPrimary
            bold: true
        }

        Item {
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
            height: 1
        }
    }

    // Bottom Keybinding Footer Bar
    Rectangle {
        id: footer
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        height: 1
        visible: !root.inlineMode && !root.standalone
        color: root.tileColor

        Text {
            x: 2
            visible: !root.showingDemo
            spans: [
                {
                    text: " ↑↓/←→ ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "select  ",
                    color: root.mutedColor
                },
                {
                    text: " Enter/Space ",
                    color: root.brandPrimary,
                    bold: true
                },
                {
                    text: "open inspector  ",
                    color: root.mutedColor
                },
                {
                    text: " F2 ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "fullscreen  ",
                    color: root.mutedColor
                },
                {
                    text: " T ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "theme  ",
                    color: root.mutedColor
                },
                {
                    text: " Q/Esc ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "quit",
                    color: root.mutedColor
                }
            ]
        }

        Text {
            x: 2
            visible: root.showingDemo
            spans: [
                {
                    text: " Esc ",
                    color: root.brandPrimary,
                    bold: true
                },
                {
                    text: "back to grid  ",
                    color: root.mutedColor
                },
                {
                    text: " Tab ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "cycle inspector controls  ",
                    color: root.mutedColor
                },
                {
                    text: " F2 ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "toggle sidebar  ",
                    color: root.mutedColor
                },
                {
                    text: " Q ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "quit",
                    color: root.mutedColor
                }
            ]
        }
    }

    // Master-Detail Body Area
    Item {
        id: bodyArea
        anchors {
            top: root.standalone ? parent.top : header.bottom
            bottom: root.standalone ? parent.bottom : footer.top
            left: parent.left
            right: parent.right
        }
        visible: !root.inlineMode

        // Left Master Sidebar: Categorized Component List
        Panel {
            id: sidebar
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
            }
            width: root.sidebarWidth
            visible: !root.fullscreenStage && !root.standalone
            title: "Components"
            titleColor: !root.showingDemo ? root.brandPrimary : root.mutedColor
            titleTrailing: (root.currentIndex + 1) + "/" + root.catalog.length
            titleTrailingColor: root.mutedColor
            color: root.tileColor
            border {
                style: !root.showingDemo ? Border.Double : Border.Single
                color: !root.showingDemo ? root.brandPrimary : root.lineColor
            }

            Item {
                anchors {
                    fill: parent
                    leftMargin: 1
                    rightMargin: 1
                    topMargin: 1
                    bottomMargin: 1
                }
                clip: true

                Repeater {
                    model: root.catalog
                    delegate: Rectangle {
                        id: navRow
                        required property int index
                        required property var modelData
                        readonly property bool isCurrent: index === root.currentIndex
                        readonly property bool isOpen: index === root.openIndex

                        visible: index >= root.sidebarScroll && index < root.sidebarScroll + root.sidebarVisibleRows
                        x: 0
                        y: index - root.sidebarScroll
                        width: parent.width
                        height: 1
                        color: isCurrent ? root.tileCurrentColor : "transparent"

                        Text {
                            x: 1
                            text: (navRow.isOpen ? "● " : (navRow.isCurrent ? "▸ " : "  ")) + navRow.modelData.name
                            color: navRow.isCurrent ? navRow.modelData.accent : root.textColor
                            bold: navRow.isCurrent
                        }

                        Text {
                            anchors {
                                right: parent.right
                                rightMargin: 1
                            }
                            visible: navRow.modelData.name.length + text.length + 5 <= navRow.width
                            text: navRow.modelData.badge
                            color: root.mutedColor
                            dim: !navRow.isCurrent
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                root.currentIndex = navRow.index;
                                root.openCurrent();
                            }
                        }
                    }
                }
            }
        }

        // Right Detail Stage: Overview & Quick-Jump Grid (when !showingDemo) OR Live Component Inspector (when showingDemo)
        Item {
            id: stageContainer
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: (root.fullscreenStage || root.standalone) ? parent.left : sidebar.right
                right: parent.right
            }

            // 1. Selected Component Spotlight + Quick-Jump Catalog (when browsing sidebar)
            ColumnLayout {
                anchors {
                    fill: parent
                    leftMargin: 1
                    rightMargin: 1
                }
                visible: !root.showingDemo
                spacing: 0

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 6
                    title: root.current ? root.current.name + "  ·  examples/" + root.current.file : "Component Spotlight"
                    titleColor: root.current ? root.current.accent : root.brandPrimary
                    titleBadge: root.current ? "[" + root.current.badge + "]" : ""
                    titleBadgeColor: root.brandPrimary
                    titleTrailing: "Enter ▸ Launch"
                    titleTrailingColor: root.brandPrimary
                    footer: "CLI: qmltui_gallery --example " + (root.current ? root.current.name : "")
                    footerColor: root.mutedColor
                    footerTrailing: "F2 Fullscreen"
                    footerTrailingColor: root.mutedColor
                    color: root.tileColor
                    border {
                        style: Border.Rounded
                        color: root.current ? root.current.accent : root.lineColor
                    }

                    Column {
                        anchors {
                            fill: parent
                            leftMargin: 2
                            rightMargin: 2
                            topMargin: 1
                            bottomMargin: 1
                        }
                        spacing: 1

                        Text {
                            text: root.current ? root.current.blurb : ""
                            color: root.textColor
                            bold: true
                        }

                        Row {
                            spacing: 2
                            Text {
                                text: "Preview:"
                                color: root.mutedColor
                            }
                            Text {
                                text: root.current ? root.current.preview : ""
                                color: root.current ? root.current.accent : root.brandPrimary
                                bold: true
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.openCurrent()
                    }
                }

                // Quick-Jump Card Grid in lower right stage
                Item {
                    id: overviewGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Repeater {
                        model: root.catalog
                        delegate: Rectangle {
                            id: card
                            required property int index
                            required property var modelData
                            readonly property int col: index % root.gridCols
                            readonly property int row: Math.floor(index / root.gridCols)
                            readonly property bool current: index === root.currentIndex

                            visible: row >= root.gridScrollRow && row < root.gridScrollRow + root.gridVisRows
                            x: col * (root.cardWidth + root.cardGap)
                            y: (row - root.gridScrollRow) * (root.cardHeight + root.cardGap)
                            width: root.cardWidth
                            height: root.cardHeight
                            clip: true
                            color: current ? root.tileCurrentColor : root.tileColor
                            border {
                                style: current ? Border.Double : Border.Single
                                color: current ? modelData.accent : root.lineColor
                            }

                            Text {
                                x: 1
                                y: 1
                                text: card.modelData.name
                                color: card.current ? card.modelData.accent : root.textColor
                                bold: card.current
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.currentIndex = card.index;
                                    root.openCurrent();
                                }
                            }
                        }
                    }
                }
            }

            // 2. Live Interactive Component Stage (when showingDemo is true)
            Item {
                id: liveStagePanel
                anchors.fill: parent
                visible: root.showingDemo

                Loader {
                    id: stageLoader
                    anchors {
                        fill: parent
                        leftMargin: 1
                        rightMargin: root.standalone ? 1 : 0
                        topMargin: root.standalone ? 1 : 0
                        bottomMargin: root.standalone ? 1 : 0
                    }
                    clip: true
                    active: root.showingDemo && !root.isInlineExample && root.current !== null
                    source: active ? "examples/" + root.current.file : ""

                    onLoaded: {
                        if (item)
                            root.focusExample(item);
                    }
                }

                Text {
                    anchors {
                        fill: parent
                        margins: 2
                    }
                    visible: stageLoader.status === Loader.Error
                    wrapMode: Text.WordWrap
                    color: "#f87171"
                    text: stageLoader.errorString
                }
            }
        }
    }

    // Inline mode host
    Loader {
        id: inlineLoader
        visible: root.inlineMode
        width: parent.width
        height: implicitHeight
        active: root.showingDemo && root.isInlineExample && root.current !== null
        source: active ? "examples/" + root.current.file : ""

        onLoaded: {
            if (item)
                root.focusExample(item);
        }
    }
}
