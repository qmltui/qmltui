pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// QmlTui Gallery: Searchable Component GridView & Interactive Example Stage.
// Screen 1: Full-width GridView of component preview tiles with an on-demand
//           inline search input (/ or click).
// Screen 2: Full-width Example container hosting the live interactive QML
//           component example (Enter/Space/click to open, Esc to return).
Terminal {
    id: root
    title: showingDemo && current ? "QmlTui Gallery · " + current.file : "QmlTui Gallery"
    color: (showingDemo && current && current.file === "TerminalCapabilities.qml") ? QmlTui.defaultColor : canvasColor

    // Palette aligned with docs/.vitepress/theme/custom.css (.dark)
    readonly property string brandPrimary: "#2cde85"
    readonly property string brandAccent: "#00ff66"
    readonly property string brandDeep: "#00a651"
    readonly property string canvasColor: "#0d1117"
    readonly property string tileColor: "#161b22"
    readonly property string tileHoverColor: "#1c2b26"
    readonly property string tileCurrentColor: "#122d24"
    readonly property string lineColor: "#30363d"
    readonly property string textColor: "#c9d1d9"
    readonly property string mutedColor: "#8b949e"

    // Search & navigation state
    property bool searchActive: false
    property string searchQuery: ""
    property int currentIndex: 0
    property int openIndex: -1

    // Live example state
    property Item liveExampleItem: null
    property string codeError: ""

    readonly property bool showingDemo: openIndex >= 0
    readonly property var current: showingDemo ? filteredCatalog[openIndex] : (filteredCatalog.length > 0 ? filteredCatalog[currentIndex] : null)

    readonly property string standaloneFile: typeof qmltuiStandaloneExample === "string" ? qmltuiStandaloneExample : ""
    readonly property bool standalone: standaloneFile !== ""
    readonly property string initialPageFile: typeof qmltuiGalleryPage === "string" ? qmltuiGalleryPage : ""
    readonly property var availableFiles: typeof qmltuiAvailableExamples !== "undefined" && qmltuiAvailableExamples !== null && typeof qmltuiAvailableExamples.length === "number" ? qmltuiAvailableExamples : null
    readonly property var exampleSources: typeof qmltuiExampleSources !== "undefined" && qmltuiExampleSources !== null ? qmltuiExampleSources : ({})

    readonly property var catalog: availableFiles ? allEntries.filter(entry => availableFiles.indexOf(entry.file) >= 0) : allEntries
    readonly property var filteredCatalog: {
        const q = searchQuery.trim().toLowerCase();
        if (q === "")
            return catalog;
        return catalog.filter(entry => {
            return entry.name.toLowerCase().indexOf(q) >= 0 || entry.badge.toLowerCase().indexOf(q) >= 0 || entry.blurb.toLowerCase().indexOf(q) >= 0 || entry.file.toLowerCase().indexOf(q) >= 0;
        });
    }

    readonly property int gridCols: Math.max(1, Math.floor(componentGrid.width / 36))

    // Ordered by module: Core (21) -> Controls (14) -> Optional: Image (2) & PtyView (1),
    // and within each module from fundamental/most frequently used items to specialized ones.
    readonly property var allEntries: [
        // 1. Core (21)
        {
            name: "Text",
            badge: "Core",
            accent: "#79c0ff",
            preview: "Bold · Italic · Underline · Wrap",
            blurb: "Styled terminal text, alignment, wrapping, and eliding",
            file: "Text.qml"
        },
        {
            name: "RichText",
            badge: "Core",
            accent: "#ffa198",
            preview: "Spans · <b>HTML</b> · <a href>Links</a>",
            blurb: "Inline styled spans, HTML tags, and clickable links",
            file: "RichText.qml"
        },
        {
            name: "Rectangle",
            badge: "Core",
            accent: "#ff7b72",
            preview: "╭─ Rounded ─╮ ╔═ Double ═╗ ▓▓▒▒░░",
            blurb: "Colored cell boxes, border styles, and gradients",
            file: "Rectangle.qml"
        },
        {
            name: "TextInput",
            badge: "Core",
            accent: "#56d4dd",
            preview: "alice█  ·  placeholder  ·  ••••••",
            blurb: "Single-line text field with placeholder and password mode",
            file: "TextInput.qml"
        },
        {
            name: "TextArea",
            badge: "Core",
            accent: "#d2a8ff",
            preview: "1  import QmlTui    2  Item { }",
            blurb: "Multi-line text editor with scrolling and selection",
            file: "TextArea.qml"
        },
        {
            name: "MouseArea",
            badge: "Core",
            accent: "#39c5cf",
            preview: "┼ (x: 24, y: 8) · hover & wheel",
            blurb: "Mouse click, hover, drag, and wheel event handling",
            file: "MouseArea.qml"
        },
        {
            name: "Anchors",
            badge: "Core",
            accent: "#58a6ff",
            preview: "top: header.bottom · left: sidebar.right",
            blurb: "Relative edge attachment, centering, and margins",
            file: "Anchors.qml"
        },
        {
            name: "Layouts",
            badge: "Core",
            accent: "#2cde85",
            preview: "RowLayout · ColumnLayout · GridLayout",
            blurb: "RowLayout, ColumnLayout, and GridLayout sizing",
            file: "Layouts.qml"
        },
        {
            name: "StackLayout",
            badge: "Core",
            accent: "#f2cc60",
            preview: "Page 1 ▸ Page 2 ▸ Page 3 (currentIndex)",
            blurb: "Indexed page container showing one child item at a time",
            file: "StackLayout.qml"
        },
        {
            name: "Keys",
            badge: "Core",
            accent: "#7ee787",
            preview: "Keys.onPressed · KeyEvent · modifiers",
            blurb: "Attached keyboard event handling and KeyEvent inspection",
            file: "Keys.qml"
        },
        {
            name: "Shortcut",
            badge: "Core",
            accent: "#d2a8ff",
            preview: "[1] Amber   [2] Mint   [3] Rose",
            blurb: "Declarative keyboard shortcuts and key sequences",
            file: "Shortcut.qml"
        },
        {
            name: "FocusScope",
            badge: "Core",
            accent: "#58a6ff",
            preview: "[1] Servers (active)   [2] Actions",
            blurb: "Scoped keyboard focus container remembering child focus",
            file: "FocusScope.qml"
        },
        {
            name: "Repeater",
            badge: "Core",
            accent: "#f2cc60",
            preview: "[alpha] [beta] [gamma] [delta]",
            blurb: "Data-driven item instantiation inside positioners",
            file: "Repeater.qml"
        },
        {
            name: "Loader",
            badge: "Core",
            accent: "#a5d6ff",
            preview: "Loader { sourceComponent: viewA }",
            blurb: "Dynamic component loading and view switching",
            file: "Loader.qml"
        },
        {
            name: "ListView",
            badge: "Core",
            accent: "#39c5cf",
            preview: "› Ada Lovelace   Analyst   ★★★★★",
            blurb: "Scrollable virtualized list with current-item selection",
            file: "ListView.qml"
        },
        {
            name: "TableView",
            badge: "Core",
            accent: "#f2cc60",
            preview: "PID   NAME       CPU%   STATE",
            blurb: "Multi-column tabular view with headers and selection",
            file: "TableView.qml"
        },
        {
            name: "TreeView",
            badge: "Core",
            accent: "#7ee787",
            preview: "▾ src/  ▾ Core/  ▸ Controls/",
            blurb: "Expandable hierarchical tree with keyboard navigation",
            file: "TreeView.qml"
        },
        {
            name: "GridView",
            badge: "Core",
            accent: "#ffa657",
            preview: "[Tile 1]  [Tile 2]  [Tile 3]",
            blurb: "Virtualized 2D cell grid with multi-column flow",
            file: "GridView.qml"
        },
        {
            name: "ScrollView",
            badge: "Core",
            accent: "#58a6ff",
            preview: "Viewport content  ·  scrollbars ▐",
            blurb: "Scrollable viewport container with scrollbars",
            file: "ScrollView.qml"
        },
        {
            name: "Popup",
            badge: "Core",
            accent: "#7ee787",
            preview: "Modal Overlay  ·  dimmed backdrop",
            blurb: "Centered overlay surface with backdrop dimming",
            file: "Popup.qml"
        },
        {
            name: "Color",
            badge: "Core",
            accent: "#f2cc60",
            preview: "default · QmlTui.ansi(0..7) · #rrggbb",
            blurb: "SGR default/named colors, ANSI palette, and 24-bit RGB",
            file: "Color.qml"
        },
        {
            name: "Gradient",
            badge: "Core",
            accent: "#38bdf8",
            preview: "Horizontal ──► · Vertical │ · 4-Stop",
            blurb: "Multi-stop horizontal and vertical linear color gradients",
            file: "Gradient.qml"
        },
        {
            name: "Clipboard",
            badge: "Core",
            accent: "#2cde85",
            preview: "Clipboard.copy(...) · OSC 52 payload",
            blurb: "System clipboard copy requests and event notifications",
            file: "Clipboard.qml"
        },
        {
            name: "Canvas",
            badge: "Core",
            accent: "#56d4dd",
            preview: "Braille 2x4  ·  HalfBlock 1x2",
            blurb: "Sub-cell pixel drawing for lines, shapes, and plots",
            file: "Canvas.qml"
        },
        {
            name: "States",
            badge: "Core",
            accent: "#d2a8ff",
            preview: "● STANDBY ──► ● ONLINE (Transition)",
            blurb: "Declarative States with animated Transitions",
            file: "States.qml"
        },
        {
            name: "Animation",
            badge: "Core",
            accent: "#ff7b72",
            preview: "Linear · OutQuad · OutBounce ───●",
            blurb: "NumberAnimation and ColorAnimation timelines",
            file: "Animation.qml"
        },
        {
            name: "Behavior",
            badge: "Core",
            accent: "#56d4dd",
            preview: "Behavior on x { NumberAnimation }",
            blurb: "Automatic property transitions when values change",
            file: "Behavior.qml"
        },
        // 2. Controls (14)
        {
            name: "Button",
            badge: "Controls",
            accent: "#2cde85",
            preview: "[ Primary ]  [ Secondary ]  [ Filled ]",
            blurb: "Push, checkable, and custom-styled terminal buttons",
            file: "Button.qml"
        },
        {
            name: "CheckBox",
            badge: "Controls",
            accent: "#79c0ff",
            preview: "[x] Logging   [ ] Verbose   [ ON] Auto",
            blurb: "Boolean toggles with default and custom indicators",
            file: "CheckBox.qml"
        },
        {
            name: "RadioButton",
            badge: "Controls",
            accent: "#f2cc60",
            preview: "(•) Fast   ( ) Balanced   ( ) Safe",
            blurb: "Mutually exclusive choices managed by ButtonGroup",
            file: "RadioButton.qml"
        },
        {
            name: "ComboBox",
            badge: "Controls",
            accent: "#2cde85",
            preview: "[ Production          ▾ ]",
            blurb: "Dropdown selector with inline value and Popup list",
            file: "ComboBox.qml"
        },
        {
            name: "SpinBox",
            badge: "Controls",
            accent: "#d2a8ff",
            preview: "[-]    42    [+]   stepSize: 1",
            blurb: "Numeric stepper with keyboard, click, and wheel control",
            file: "SpinBox.qml"
        },
        {
            name: "Slider",
            badge: "Controls",
            accent: "#d2a8ff",
            preview: "──────●──────────  42 / 100",
            blurb: "Stepped value track controlled by arrow keys or mouse",
            file: "Slider.qml"
        },
        {
            name: "ProgressBar",
            badge: "Controls",
            accent: "#58a6ff",
            preview: "▰▰▰▰▰▰▱▱▱▱  64%  [████░░]",
            blurb: "Determinate and styled progress indicators",
            file: "ProgressBar.qml"
        },
        {
            name: "Panel",
            badge: "Controls",
            accent: "#7ee787",
            preview: "╭─ Title ────────────── info ─╮",
            blurb: "Titled container frame with header and footer slots",
            file: "Panel.qml"
        },
        {
            name: "Dialog",
            badge: "Controls",
            accent: "#ff7b72",
            preview: "Confirm Action?   [ OK ] [Cancel]",
            blurb: "Modal confirmation dialog with standard buttons",
            file: "Dialog.qml"
        },
        {
            name: "TabBar",
            badge: "Controls",
            accent: "#f2cc60",
            preview: "Overview  ·  Telemetry  ·  Logs",
            blurb: "Horizontal tab selector driving multi-page views",
            file: "TabBar.qml"
        },
        {
            name: "StackView",
            badge: "Controls",
            accent: "#d2a8ff",
            preview: "Page 1 ▸ Page 2 ▸ Page 3 (depth 3)",
            blurb: "Push/pop navigation stack for multi-step pages",
            file: "StackView.qml"
        },
        {
            name: "BusyIndicator",
            badge: "Controls",
            accent: "#ffa657",
            preview: "⠋ Dots   | Line   ▖ Block   [=  ]",
            blurb: "Animated terminal activity spinner with custom frames",
            file: "BusyIndicator.qml"
        },
        {
            name: "ToolTip",
            badge: "Controls",
            accent: "#f2cc60",
            preview: "▸ Compile the current project",
            blurb: "Contextual help popover attached to a target item",
            file: "ToolTip.qml"
        },
        {
            name: "Sparkline",
            badge: "Controls",
            accent: "#56d4dd",
            preview: "▁▂▄▆█▆▄▃▅▇█▆▄▂  cpu %",
            blurb: "Compact inline data trend chart using block glyphs",
            file: "Sparkline.qml"
        },
        // 3. Optional Items: Image (2) & PtyView (1)
        {
            name: "Image",
            badge: "Image",
            accent: "#f2cc60",
            preview: "⣿ ▀▄ # · Braille · HalfBlock · ASCII",
            blurb: "Terminal image rendering with cell and protocol modes",
            file: "Image.qml"
        },
        {
            name: "AnimatedSprite",
            badge: "Image",
            accent: "#ffa657",
            preview: "▸ [1/4] · 256x64 sprite sheet",
            blurb: "Frame-by-frame sprite sheet animation playback",
            file: "AnimatedSprite.qml"
        },
        {
            name: "PtyView",
            badge: "PtyView",
            accent: "#7ee787",
            preview: "[sh] $ echo hello",
            blurb: "Embedded Unix pseudo-terminal process viewport",
            file: "PtyView.qml"
        },
        {
            name: "TerminalCapabilities",
            badge: "Core",
            accent: "#58a6ff",
            preview: "TrueColor · SyncOutput · Kitty · Sixel",
            blurb: "Read-only inspection of host terminal capabilities",
            file: "TerminalCapabilities.qml"
        }
    ]

    function badgeColor(badge) {
        if (badge === "Controls")
            return root.brandPrimary;
        if (badge === "Image")
            return "#f2cc60";
        if (badge === "PtyView")
            return "#d2a8ff";
        return "#58a6ff";
    }

    function ensureVisible() {
        if (filteredCatalog.length === 0) {
            componentGrid.contentY = 0;
            return;
        }
        if (currentIndex >= filteredCatalog.length)
            currentIndex = Math.max(0, filteredCatalog.length - 1);
        const cols = Math.max(1, Math.floor(componentGrid.width / Math.max(1, componentGrid.cellWidth)));
        const row = Math.floor(currentIndex / cols);
        const topY = row * componentGrid.cellHeight;
        const bottomY = topY + componentGrid.cellHeight;
        if (topY < componentGrid.contentY)
            componentGrid.contentY = topY;
        else if (bottomY > componentGrid.contentY + componentGrid.height)
            componentGrid.contentY = Math.max(0, bottomY - componentGrid.height);
    }

    function move(delta) {
        if (filteredCatalog.length === 0)
            return;
        const next = Math.max(0, Math.min(filteredCatalog.length - 1, currentIndex + delta));
        currentIndex = next;
    }

    function activateSearch() {
        searchActive = true;
        searchInput.focus = true;
    }

    function deactivateSearch(clearFilter) {
        if (clearFilter) {
            searchInput.text = "";
            searchQuery = "";
        }
        searchActive = false;
        root.focus = true;
    }

    function exampleHasActiveFocus() {
        return (function check(node) {
                if (!node || !node.visible)
                    return false;
                if (node.activeFocus)
                    return true;
                for (let i = 0; i < node.children.length; ++i) {
                    if (check(node.children[i]))
                        return true;
                }
                return false;
            })(demoHost);
    }

    function focusExample(item) {
        if (!item) {
            demoHost.focusPolicy = Qt.StrongFocus;
            demoHost.focus = true;
            return;
        }
        if ((item.focusPolicy & Qt.TabFocus) && item.visible && item.enabled) {
            item.focus = true;
            return;
        }
        let fallback = null;
        const marked = (function visit(node) {
                for (let i = 0; i < node.children.length; ++i) {
                    const child = node.children[i];
                    if (child.focus && child.visible && child.enabled)
                        return child;
                    if (!fallback && (child.focusPolicy & Qt.TabFocus) && child.visible && child.enabled)
                        fallback = child;
                    const found = visit(child);
                    if (found)
                        return found;
                }
                return null;
            })(item);
        const target = marked || fallback;
        if (target) {
            if (target.focusPolicy === Qt.NoFocus)
                target.focusPolicy = Qt.StrongFocus;
            target.focus = true;
        } else {
            if (item.focusPolicy === Qt.NoFocus)
                item.focusPolicy = Qt.StrongFocus;
            item.focus = true;
        }
    }

    function destroyLiveExample() {
        if (liveExampleItem !== null) {
            const old = liveExampleItem;
            liveExampleItem = null;
            old.visible = false;
            old.enabled = false;
            old.destroy();
        }
        codeError = "";
    }

    function loadCurrentExample() {
        if (!showingDemo || current === null)
            return;
        destroyLiveExample();
        const src = exampleSources[current.file] ? String(exampleSources[current.file]) : "";
        try {
            let obj = null;
            if (src !== "") {
                obj = Qt.createQmlObject(src, demoHost, "qrc:/qt/qml/Gallery/examples/" + current.file);
            } else {
                const comp = Qt.createComponent("examples/" + current.file);
                if (comp.status === Component.Ready) {
                    obj = comp.createObject(demoHost);
                } else {
                    codeError = comp.errorString();
                    return;
                }
            }
            if (obj) {
                liveExampleItem = obj;
                if (!obj.anchors.fill && !obj.anchors.centerIn && !obj.anchors.left && !obj.anchors.right && !obj.anchors.top && !obj.anchors.bottom && !obj.anchors.horizontalCenter && !obj.anchors.verticalCenter) {
                    obj.anchors.centerIn = demoHost;
                }
                codeError = "";
                focusExample(obj);
            } else {
                codeError = "Failed to instantiate component from source.";
            }
        } catch (err) {
            codeError = err && err.message ? String(err.message) : String(err);
        }
    }

    function openCurrent() {
        if (filteredCatalog.length === 0)
            return;
        if (searchActive)
            searchActive = false;
        openIndex = currentIndex;
        loadCurrentExample();
    }

    function closeDemo() {
        if (standalone) {
            Qt.quit();
            return;
        }
        destroyLiveExample();
        openIndex = -1;
        root.focus = true;
        ensureVisible();
    }

    Component.onCompleted: {
        const targetFile = standalone ? standaloneFile : initialPageFile;
        if (!targetFile)
            return;
        const index = filteredCatalog.findIndex(entry => entry.file === targetFile);
        if (index >= 0) {
            currentIndex = index;
            openCurrent();
        }
    }

    onCurrentIndexChanged: ensureVisible()
    onGridColsChanged: ensureVisible()

    Keys.onPressed: event => {
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
        if (searchActive && searchInput.activeFocus)
            return;

        switch (event.key) {
        case Qt.Key_Slash:
            activateSearch();
            break;
        case Qt.Key_Left:
        case Qt.Key_H:
            move(-1);
            break;
        case Qt.Key_Right:
        case Qt.Key_L:
            move(1);
            break;
        case Qt.Key_Up:
        case Qt.Key_K:
            move(-gridCols);
            break;
        case Qt.Key_Down:
        case Qt.Key_J:
            move(gridCols);
            break;
        case Qt.Key_PageUp:
            move(-gridCols * 3);
            break;
        case Qt.Key_PageDown:
            move(gridCols * 3);
            break;
        case Qt.Key_Home:
            currentIndex = 0;
            break;
        case Qt.Key_End:
            currentIndex = Math.max(0, filteredCatalog.length - 1);
            break;
        case Qt.Key_Return:
        case Qt.Key_Enter:
        case Qt.Key_Space:
            openCurrent();
            break;
        case Qt.Key_Escape:
            if (searchQuery !== "" || searchActive) {
                deactivateSearch(true);
            } else {
                Qt.quit();
            }
            break;
        case Qt.Key_Q:
            Qt.quit();
            break;
        default:
            return;
        }
        event.accepted = true;
    }

    // Top Application Header Bar
    Rectangle {
        id: header
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 1
        visible: !root.standalone
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
                    color: root.brandDeep
                }
            }
        }

        Text {
            anchors {
                left: brandTitle.right
                leftMargin: 2
                right: parent.right
                rightMargin: 14
            }
            y: 0
            clip: true
            visible: root.current !== null
            spans: [
                {
                    text: " · ",
                    color: root.mutedColor
                },
                {
                    text: root.current ? root.current.name : "",
                    color: root.current ? root.current.accent : root.brandAccent,
                    bold: true
                },
                {
                    text: " [" + (root.current ? root.current.badge : "") + "] ",
                    color: root.current ? root.badgeColor(root.current.badge) : root.brandPrimary
                },
                {
                    text: root.showingDemo ? " · examples/" + (root.current ? root.current.file : "") + "  ◂ Esc back to grid" : "— " + (root.current ? root.current.blurb : ""),
                    color: root.mutedColor
                }
            ]

            MouseArea {
                anchors.fill: parent
                enabled: root.showingDemo
                onClicked: root.closeDemo()
            }
        }

        Text {
            anchors {
                right: parent.right
                rightMargin: 2
            }
            y: 0
            text: root.filteredCatalog.length > 0 ? (root.currentIndex + 1) + " / " + root.filteredCatalog.length : "0 / 0"
            color: root.brandPrimary
            bold: true
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
        visible: !root.standalone
        color: root.tileColor

        Text {
            x: 2
            visible: !root.showingDemo
            spans: [
                {
                    text: " ↑↓←→ ",
                    color: root.brandPrimary,
                    bold: true
                },
                {
                    text: "navigate  ",
                    color: root.mutedColor
                },
                {
                    text: " Enter/Space ",
                    color: root.brandAccent,
                    bold: true
                },
                {
                    text: "open example  ",
                    color: root.mutedColor
                },
                {
                    text: " / ",
                    color: root.brandPrimary,
                    bold: true
                },
                {
                    text: "search  ",
                    color: root.mutedColor
                },
                {
                    text: " Q/Esc ",
                    color: root.brandPrimary,
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
                    color: root.brandAccent,
                    bold: true
                },
                {
                    text: "back to grid  ",
                    color: root.mutedColor
                },
                {
                    text: " Tab/Arrows ",
                    color: root.brandPrimary,
                    bold: true
                },
                {
                    text: "interact  ",
                    color: root.mutedColor
                },
                {
                    text: " Q ",
                    color: root.brandPrimary,
                    bold: true
                },
                {
                    text: "quit",
                    color: root.mutedColor
                }
            ]
        }
    }

    // Main Content Area
    Item {
        id: bodyArea
        anchors {
            top: root.standalone ? parent.top : header.bottom
            topMargin: root.standalone ? 0 : 1
            bottom: root.standalone ? parent.bottom : footer.top
            left: parent.left
            right: parent.right
        }

        // Screen 1: Borderless Component GridView with inline search bar
        Item {
            id: catalogPanel
            anchors.fill: parent
            visible: !root.showingDemo

            // Top subheader row: component count on left, inline search on right
            Text {
                x: 2
                y: 0
                spans: [
                    {
                        text: "Components ",
                        color: root.brandPrimary,
                        bold: true
                    },
                    {
                        text: "(" + root.filteredCatalog.length + (root.searchQuery !== "" ? "/" + root.catalog.length : "") + ")",
                        color: root.mutedColor
                    }
                ]
            }

            Item {
                id: searchHeaderSlot
                anchors {
                    right: parent.right
                    rightMargin: 2
                }
                y: 0
                implicitWidth: root.searchActive ? 26 : searchTriggerLabel.implicitWidth
                width: implicitWidth
                height: 1

                // Inactive compact trigger
                Text {
                    id: searchTriggerLabel
                    visible: !root.searchActive
                    text: root.searchQuery !== "" ? "/ " + root.searchQuery + " ×" : "/ Search"
                    color: root.searchQuery !== "" ? root.brandAccent : root.brandPrimary
                    bold: root.searchQuery !== ""

                    MouseArea {
                        anchors.fill: parent
                        onClicked: mouse => {
                            if (root.searchQuery !== "" && mouse.x >= parent.width - 2) {
                                root.deactivateSearch(true);
                            } else {
                                root.activateSearch();
                            }
                        }
                    }
                }

                // Active inline search TextInput
                Row {
                    visible: root.searchActive
                    spacing: 1

                    Text {
                        text: "/"
                        color: root.brandAccent
                        bold: true
                    }

                    TextInput {
                        id: searchInput
                        width: 21
                        height: 1
                        enabled: root.searchActive
                        visible: root.searchActive
                        placeholderText: "Filter components..."
                        color: root.textColor

                        onTextChanged: {
                            if (root.searchQuery !== text) {
                                root.searchQuery = text;
                                root.currentIndex = 0;
                                root.ensureVisible();
                            }
                        }

                        Keys.onPressed: event => {
                            if (event.key === Qt.Key_Escape) {
                                root.deactivateSearch(true);
                                event.accepted = true;
                            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                if (root.filteredCatalog.length > 0) {
                                    root.openCurrent();
                                } else {
                                    root.deactivateSearch(false);
                                }
                                event.accepted = true;
                            } else if (event.key === Qt.Key_Down) {
                                root.move(root.gridCols);
                                event.accepted = true;
                            } else if (event.key === Qt.Key_Up) {
                                root.move(-root.gridCols);
                                event.accepted = true;
                            }
                        }
                    }

                    Text {
                        text: "×"
                        color: root.mutedColor
                        bold: true

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.deactivateSearch(true)
                        }
                    }
                }
            }

            GridView {
                id: componentGrid
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                    leftMargin: 2
                    rightMargin: 1
                    topMargin: 2
                    bottomMargin: 0
                }
                clip: true
                cellWidth: Math.max(26, Math.floor(width / root.gridCols))
                cellHeight: 4
                model: root.filteredCatalog

                delegate: Item {
                    id: tileItem
                    required property int index
                    required property var modelData
                    readonly property bool isCurrent: index === root.currentIndex
                    readonly property bool isHovered: tileMouse.containsMouse

                    width: componentGrid.cellWidth
                    height: componentGrid.cellHeight

                    Rectangle {
                        id: tileCard
                        width: Math.max(10, parent.width - 2)
                        height: parent.height - 1
                        clip: true
                        color: tileItem.isCurrent ? root.tileCurrentColor : (tileItem.isHovered ? root.tileHoverColor : root.tileColor)

                        // Row 1: Component Name + Module Badge
                        Text {
                            x: 2
                            y: 0
                            text: (tileItem.isCurrent ? "▸ " : (tileItem.isHovered ? "› " : "")) + tileItem.modelData.name
                            color: tileItem.isCurrent ? root.brandAccent : (tileItem.isHovered ? root.brandPrimary : root.textColor)
                            bold: true
                        }

                        Text {
                            anchors {
                                right: parent.right
                                rightMargin: 2
                            }
                            y: 0
                            text: "[" + tileItem.modelData.badge + "]"
                            color: root.badgeColor(tileItem.modelData.badge)
                            bold: tileItem.isCurrent || tileItem.isHovered
                        }

                        // Row 2: Representative Component Preview Content
                        Text {
                            x: 2
                            y: 1
                            width: Math.max(0, parent.width - 4)
                            clip: true
                            text: tileItem.modelData.preview
                            color: tileItem.modelData.accent
                            bold: tileItem.isCurrent || tileItem.isHovered
                        }

                        // Row 3: Concise Description Blurb
                        Text {
                            x: 2
                            y: 2
                            width: Math.max(0, parent.width - 4)
                            clip: true
                            text: tileItem.modelData.blurb
                            color: (tileItem.isCurrent || tileItem.isHovered) ? root.textColor : root.mutedColor
                        }

                        MouseArea {
                            id: tileMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                root.currentIndex = tileItem.index;
                                root.openCurrent();
                            }
                        }
                    }
                }
            }

            // Empty search result feedback
            Text {
                anchors.centerIn: parent
                visible: root.filteredCatalog.length === 0
                text: "No components match \"" + root.searchQuery + "\" — press Esc to clear filter"
                color: root.mutedColor
                bold: true
            }
        }

        // Screen 2: Full-Width Example Stage
        MouseArea {
            id: examplePaneArea
            anchors {
                fill: parent
                leftMargin: 1
                rightMargin: 1
            }
            visible: root.showingDemo
            onPressed: {
                if (!root.exampleHasActiveFocus())
                    root.focusExample(root.liveExampleItem);
            }

            Item {
                id: examplePanel
                anchors.fill: parent

                Item {
                    id: demoHost
                    anchors {
                        top: parent.top
                        left: parent.left
                        right: parent.right
                        bottom: errorBanner.visible ? errorBanner.top : parent.bottom
                        leftMargin: 1
                        rightMargin: 1
                        topMargin: 0
                        bottomMargin: 0
                    }
                    clip: true
                }

                Rectangle {
                    id: errorBanner
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                        leftMargin: 1
                        rightMargin: 1
                        bottomMargin: 1
                    }
                    visible: root.codeError !== ""
                    height: visible ? Math.min(6, Math.max(3, errorText.implicitHeight + 2)) : 0
                    color: "#2d1215"

                    Text {
                        id: errorText
                        anchors {
                            fill: parent
                            leftMargin: 1
                            rightMargin: 1
                            topMargin: 1
                            bottomMargin: 1
                        }
                        wrapMode: Text.WordWrap
                        color: "#ffa198"
                        text: root.codeError
                    }
                }
            }
        }
    }
}
