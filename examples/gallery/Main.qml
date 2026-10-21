pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// QmlTui Gallery: Searchable Component GridView & Live Editable Code Playground.
// Screen 1: Full-width Panel with a GridView of component preview tiles and an
//           on-demand search input in the Panel's titleTrailing slot (/ or click).
// Screen 2: Side-by-side split view with the Gallery-owned Example container
//           Panel on the left and a live-editable QML Code Panel on the right
//           that reloads the example 500ms after typing stops.
Terminal {
    id: root
    title: showingDemo && current ? "QmlTui Gallery · " + current.file : "QmlTui Gallery"
    color: "#161b22"

    readonly property string brandPrimary: "#2cde85"
    readonly property string brandAccent: "#00ff66"
    readonly property string tileColor: "#161b22"
    readonly property string tileCurrentColor: "#12261e"
    readonly property string lineColor: "#30363d"
    readonly property string textColor: "#c9d1d9"
    readonly property string mutedColor: "#8b949e"

    // Search & navigation state
    property bool searchActive: false
    property string searchQuery: ""
    property int currentIndex: 0
    property int openIndex: -1
    property bool fullscreenStage: false

    // Live example & editable code state
    property Item liveExampleItem: null
    property bool loadingExampleSource: false
    property string codeError: ""
    readonly property int editorLineCount: Math.max(1, codeEditor.text.split("\n").length)
    readonly property int editorCursorLine: {
        const pos = Math.max(0, Math.min(codeEditor.cursorPosition, codeEditor.text.length));
        let line = 0;
        for (let i = 0; i < pos; ++i) {
            if (codeEditor.text[i] === "\n")
                line++;
        }
        return line;
    }
    readonly property int editorCursorCol: {
        const pos = Math.max(0, Math.min(codeEditor.cursorPosition, codeEditor.text.length));
        const lastNewline = codeEditor.text.lastIndexOf("\n", pos - 1);
        return pos - (lastNewline + 1);
    }
    readonly property int gutterDigits: Math.max(2, String(editorLineCount).length)
    readonly property int gutterWidth: gutterDigits + 2

    readonly property bool showingDemo: openIndex >= 0
    readonly property var current: showingDemo ? filteredCatalog[openIndex] : (filteredCatalog.length > 0 ? filteredCatalog[currentIndex] : null)

    readonly property string standaloneFile: typeof qmltuiStandaloneExample === "string" ? qmltuiStandaloneExample : ""
    readonly property bool standalone: standaloneFile !== ""
    readonly property string initialPageFile: typeof qmltuiGalleryPage === "string" ? qmltuiGalleryPage : ""
    readonly property var availableFiles: typeof qmltuiAvailableExamples !== "undefined" && qmltuiAvailableExamples !== null && typeof qmltuiAvailableExamples.length === "number" ? qmltuiAvailableExamples : null
    readonly property var exampleSources: typeof qmltuiExampleSources !== "undefined" && qmltuiExampleSources !== null ? qmltuiExampleSources : ({})

    readonly property var categoryNames: ["All", "Controls", "Layout", "Display", "Views", "Input", "Animation", "CLI"]
    readonly property string selectedCategory: categoryNames[categoryTabs.currentIndex] || "All"

    readonly property var catalog: availableFiles ? allEntries.filter(entry => availableFiles.indexOf(entry.file) >= 0) : allEntries
    readonly property var filteredCatalog: {
        const cat = selectedCategory;
        const q = searchQuery.trim().toLowerCase();
        return catalog.filter(entry => {
            if (cat !== "All" && entry.badge !== cat)
                return false;
            if (q === "")
                return true;
            return entry.name.toLowerCase().indexOf(q) >= 0 || entry.badge.toLowerCase().indexOf(q) >= 0 || entry.blurb.toLowerCase().indexOf(q) >= 0 || entry.file.toLowerCase().indexOf(q) >= 0;
        });
    }

    readonly property int gridCols: Math.max(1, Math.floor(componentGrid.width / 36))

    readonly property var qmlKeywords: ({
            "pragma": true,
            "import": true,
            "property": true,
            "readonly": true,
            "required": true,
            "signal": true,
            "function": true,
            "component": true,
            "default": true,
            "alias": true,
            "as": true,
            "on": true,
            "if": true,
            "else": true,
            "for": true,
            "while": true,
            "return": true,
            "var": true,
            "let": true,
            "const": true,
            "true": true,
            "false": true,
            "null": true,
            "undefined": true,
            "id": true
        })

    readonly property var allEntries: [
        // 1. Controls (14)
        {
            name: "Button",
            badge: "Controls",
            accent: "#2cde85",
            preview: "[ Primary ]  [ Filled ]  [ Disabled ]",
            blurb: "Push, checkable, and custom-colored terminal buttons",
            file: "Button.qml"
        },
        {
            name: "CheckBox",
            badge: "Controls",
            accent: "#7ee0ff",
            preview: "[x] Verbose   [x] Colors   [ ] Dry-run",
            blurb: "Boolean option toggles with live state bindings",
            file: "CheckBox.qml"
        },
        {
            name: "RadioButton",
            badge: "Controls",
            accent: "#f9c74f",
            preview: "(•) JSON   ( ) YAML   ( ) Table",
            blurb: "Mutually exclusive choices managed by ButtonGroup",
            file: "RadioButton.qml"
        },
        {
            name: "Switch",
            badge: "Controls",
            accent: "#ff9f6e",
            preview: "[ ON] Dark mode   [OFF] Telemetry",
            blurb: "Binary on/off toggle switches for settings",
            file: "Switch.qml"
        },
        {
            name: "Slider",
            badge: "Controls",
            accent: "#c084fc",
            preview: "──────●──────────  50 / 100",
            blurb: "Stepped value track controlled by arrow keys or mouse",
            file: "Slider.qml"
        },
        {
            name: "ProgressBar",
            badge: "Controls",
            accent: "#38bdf8",
            preview: "▰▰▰▰▰▰▱▱▱▱  64%  [████░░]",
            blurb: "Determinate and indeterminate progress indicators",
            file: "ProgressBar.qml"
        },
        {
            name: "BusyIndicator",
            badge: "Controls",
            accent: "#f472b6",
            preview: "⠋ Loading...   ◐ Syncing...",
            blurb: "Animated terminal activity spinner with custom frames",
            file: "BusyIndicator.qml"
        },
        {
            name: "TabBar",
            badge: "Controls",
            accent: "#fbbf24",
            preview: "│ Overview │ Network │ Logs │",
            blurb: "Horizontal tab selector driving multi-page views",
            file: "TabBar.qml"
        },
        {
            name: "Panel",
            badge: "Controls",
            accent: "#94a3b8",
            preview: "╭─ Title ────────────── info ─╮",
            blurb: "Titled container frame with header and footer slots",
            file: "Panel.qml"
        },
        {
            name: "Dialog",
            badge: "Controls",
            accent: "#f87171",
            preview: "╔═ Confirm ═╗  [ OK ] [Cancel]",
            blurb: "Modal confirmation dialog with standard buttons",
            file: "Dialog.qml"
        },
        {
            name: "Popup",
            badge: "Controls",
            accent: "#a3e635",
            preview: "╭── Modal Overlay ──╮",
            blurb: "Centered overlay surface with backdrop dimming",
            file: "Popup.qml"
        },
        {
            name: "Toast",
            badge: "Controls",
            accent: "#34d399",
            preview: "┌ ✓ Build succeeded ──────┐",
            blurb: "Auto-dismissing corner notification banner",
            file: "Toast.qml"
        },
        {
            name: "ToolTip",
            badge: "Controls",
            accent: "#fcd34d",
            preview: "┌ Hint: Press Space ┐",
            blurb: "Contextual help popover attached to a target item",
            file: "ToolTip.qml"
        },
        {
            name: "StackView",
            badge: "Controls",
            accent: "#e879f9",
            preview: "Home ▸ Details ▸ Confirm (depth 3)",
            blurb: "Push/pop navigation stack for multi-step pages",
            file: "StackView.qml"
        },
        // 2. Layout (4)
        {
            name: "Rectangle",
            badge: "Layout",
            accent: "#ec4899",
            preview: "╭─ Rounded ─╮ ╔═ Double ═╗ ▓▓▒▒░░",
            blurb: "Colored cell boxes, border styles, and gradients",
            file: "Rectangle.qml"
        },
        {
            name: "Layouts",
            badge: "Layout",
            accent: "#14b8a6",
            preview: "┌── Row / Column / GridLayout ──┐",
            blurb: "RowLayout, ColumnLayout, and GridLayout sizing",
            file: "Layouts.qml"
        },
        {
            name: "Repeater",
            badge: "Layout",
            accent: "#eab308",
            preview: "[Node 1] [Node 2] [Node 3] (+Add)",
            blurb: "Data-driven item instantiation inside positioners",
            file: "Repeater.qml"
        },
        {
            name: "Loader",
            badge: "Layout",
            accent: "#6366f1",
            preview: "Loader { sourceComponent: viewA }",
            blurb: "Dynamic component loading and view switching",
            file: "Loader.qml"
        },
        // 3. Display (6)
        {
            name: "Text",
            badge: "Display",
            accent: "#e2e8f0",
            preview: "Bold · Italic · Underline · Wrap",
            blurb: "Styled terminal text, alignment, wrapping, and eliding",
            file: "Text.qml"
        },
        {
            name: "RichText",
            badge: "Display",
            accent: "#fda4af",
            preview: "Spans · <b>HTML</b> · \\x1b[32mANSI",
            blurb: "Inline styled spans, HTML tags, links, and ANSI escapes",
            file: "RichText.qml"
        },
        {
            name: "Sparkline",
            badge: "Display",
            accent: "#60a5fa",
            preview: "▁▂▄▆█▆▄▃▅▇█▆▄▂  CPU 64%",
            blurb: "Compact inline data trend chart using block glyphs",
            file: "Sparkline.qml"
        },
        {
            name: "Canvas",
            badge: "Display",
            accent: "#10b981",
            preview: "╭─ Braille 2x4 & HalfBlock 1x2 ─╮",
            blurb: "Sub-cell pixel drawing for lines, shapes, and plots",
            file: "Canvas.qml"
        },
        {
            name: "Image",
            badge: "Display",
            accent: "#f59e0b",
            preview: "⣿ ▀▄ # · Braille · HalfBlock · ASCII",
            blurb: "Terminal image rendering with cell and protocol modes",
            file: "Image.qml"
        },
        {
            name: "AnimatedSprite",
            badge: "Display",
            accent: "#ec4899",
            preview: "▸ [1/4] · 256x64 sprite sheet",
            blurb: "Frame-by-frame sprite sheet animation playback",
            file: "AnimatedSprite.qml"
        },
        // 4. Views (5)
        {
            name: "ListView",
            badge: "Views",
            accent: "#2dd4bf",
            preview: "▸ #1 Build Core    [DONE]  1.2s",
            blurb: "Scrollable virtualized list with current-item selection",
            file: "ListView.qml"
        },
        {
            name: "GridView",
            badge: "Views",
            accent: "#f43f5e",
            preview: "┌Tile 1┐ ┌Tile 2┐ ┌Tile 3┐",
            blurb: "Virtualized 2D cell grid with multi-column flow",
            file: "GridView.qml"
        },
        {
            name: "TableView",
            badge: "Views",
            accent: "#fb923c",
            preview: "PID │ NAME     │ CPU% │ STATE",
            blurb: "Multi-column tabular view with headers and selection",
            file: "TableView.qml"
        },
        {
            name: "TreeView",
            badge: "Views",
            accent: "#84cc16",
            preview: "▾ src/  ▾ QmlTui/  ▸ Controls/",
            blurb: "Expandable hierarchical tree with keyboard navigation",
            file: "TreeView.qml"
        },
        {
            name: "ScrollView",
            badge: "Views",
            accent: "#38bdf8",
            preview: "│ Viewport content         ▐│",
            blurb: "Scrollable viewport container with scrollbars",
            file: "ScrollView.qml"
        },
        // 5. Input (4)
        {
            name: "TextInput",
            badge: "Input",
            accent: "#67e8f9",
            preview: "┌ alice█────────────────────┐",
            blurb: "Single-line text field with placeholder and password mode",
            file: "TextInput.qml"
        },
        {
            name: "TextArea",
            badge: "Input",
            accent: "#818cf8",
            preview: "1 │ import QmlTui   2 │ Item { }",
            blurb: "Multi-line text editor with line numbers and highlighting",
            file: "TextArea.qml"
        },
        {
            name: "MouseArea",
            badge: "Input",
            accent: "#06b6d4",
            preview: "┼ (x: 24, y: 8) · hover & wheel",
            blurb: "Mouse click, hover, drag, and wheel event handling",
            file: "MouseArea.qml"
        },
        {
            name: "Shortcut",
            badge: "Input",
            accent: "#8b5cf6",
            preview: "[1] Build   [2] Test   [3] Deploy",
            blurb: "Declarative keyboard shortcuts and key sequences",
            file: "Shortcut.qml"
        },
        // 6. Animation (3)
        {
            name: "States",
            badge: "Animation",
            accent: "#d946ef",
            preview: "idle ──► running ──► alert",
            blurb: "Declarative States with animated Transitions",
            file: "States.qml"
        },
        {
            name: "Animation",
            badge: "Animation",
            accent: "#f43f5e",
            preview: "Linear · OutQuad · OutBounce ───●",
            blurb: "NumberAnimation and ColorAnimation timelines",
            file: "Animation.qml"
        },
        {
            name: "Behavior",
            badge: "Animation",
            accent: "#22d3ee",
            preview: "Behavior on x { NumberAnimation }",
            blurb: "Automatic property transitions when values change",
            file: "Behavior.qml"
        },
        // 7. CLI (2)
        {
            name: "TerminalCapabilities",
            badge: "CLI",
            accent: "#38bdf8",
            preview: "TrueColor · SyncOutput · Kitty · Sixel",
            blurb: "Read-only inspection of host terminal capabilities",
            file: "TerminalCapabilities.qml"
        },
        {
            name: "PtyView",
            badge: "CLI",
            accent: "#a6e3a1",
            preview: "┌ [sh] $ echo hello ──────────┐",
            blurb: "Embedded Unix pseudo-terminal process viewport",
            file: "PtyView.qml"
        }
    ]

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
                if (!node)
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
        codeEditor.focus = false;
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
        reloadTimer.stop();
        if (liveExampleItem !== null) {
            const old = liveExampleItem;
            liveExampleItem = null;
            old.visible = false;
            old.parent = null;
            old.destroy();
        }
        codeError = "";
    }

    function reloadLiveExample(qmlSource) {
        if (!showingDemo || current === null)
            return;
        const editorHadFocus = codeEditor.activeFocus;
        if (liveExampleItem !== null) {
            const old = liveExampleItem;
            liveExampleItem = null;
            old.visible = false;
            old.parent = null;
            old.destroy();
        }
        try {
            const url = "qrc:/qt/qml/Gallery/examples/" + current.file;
            const obj = Qt.createQmlObject(qmlSource, demoHost, url);
            if (obj) {
                liveExampleItem = obj;
                obj.x = 0;
                obj.y = 0;
                obj.width = Qt.binding(() => demoHost.width);
                obj.height = Qt.binding(() => demoHost.height);
                codeError = "";
                if (editorHadFocus)
                    codeEditor.focus = true;
                else
                    focusExample(obj);
            } else {
                codeError = "Failed to instantiate component from source.";
                if (editorHadFocus)
                    codeEditor.focus = true;
            }
        } catch (err) {
            const raw = err && err.message ? String(err.message) : String(err);
            codeError = raw;
            if (editorHadFocus)
                codeEditor.focus = true;
        }
    }

    function applyQmlSyntaxHighlight() {
        const text = codeEditor.text;
        const len = text.length;
        let ranges = [];
        let i = 0;
        const commentColor = "#8b949e";
        const stringColor = "#a5d6ff";
        const numberColor = "#f2cc60";
        const keywordColor = "#ff7b72";
        const typeColor = "#7ee787";

        while (i < len) {
            const ch = text[i];
            if (ch === " " || ch === "\t" || ch === "\n" || ch === "\r") {
                i++;
                continue;
            }
            if (ch === "/" && i + 1 < len && text[i + 1] === "/") {
                const start = i;
                i += 2;
                while (i < len && text[i] !== "\n")
                    i++;
                ranges.push({
                    start: start,
                    length: i - start,
                    color: commentColor,
                    italic: true
                });
                continue;
            }
            if (ch === "\"" || ch === "'") {
                const quote = ch;
                const start = i++;
                while (i < len) {
                    if (text[i] === "\\")
                        i += 2;
                    else if (text[i] === quote) {
                        i++;
                        break;
                    } else
                        i++;
                }
                i = Math.min(i, len);
                ranges.push({
                    start: start,
                    length: i - start,
                    color: stringColor
                });
                continue;
            }
            if (ch >= "0" && ch <= "9") {
                const start = i;
                while (i < len && ((text[i] >= "0" && text[i] <= "9") || text[i] === "."))
                    i++;
                ranges.push({
                    start: start,
                    length: i - start,
                    color: numberColor
                });
                continue;
            }
            if ((ch >= "a" && ch <= "z") || (ch >= "A" && ch <= "Z") || ch === "_") {
                const start = i;
                while (i < len) {
                    const c = text[i];
                    if ((c >= "a" && c <= "z") || (c >= "A" && c <= "Z") || (c >= "0" && c <= "9") || c === "_")
                        i++;
                    else
                        break;
                }
                const word = text.substring(start, i);
                if (qmlKeywords[word] === true) {
                    ranges.push({
                        start: start,
                        length: i - start,
                        color: keywordColor,
                        bold: true
                    });
                } else if (word[0] >= "A" && word[0] <= "Z") {
                    ranges.push({
                        start: start,
                        length: i - start,
                        color: typeColor,
                        bold: true
                    });
                }
                continue;
            }
            i++;
        }
        codeEditor.setHighlightLayer("syntax", ranges, codeEditor.textRevision, 0);
    }

    function loadCurrentExample() {
        if (!showingDemo || current === null)
            return;
        reloadTimer.stop();
        const src = exampleSources[current.file] ? String(exampleSources[current.file]) : "";
        loadingExampleSource = true;
        codeEditor.text = src;
        loadingExampleSource = false;
        applyQmlSyntaxHighlight();
        reloadLiveExample(src);
    }

    function resetCurrentCode() {
        if (!showingDemo || current === null)
            return;
        loadCurrentExample();
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

    Timer {
        id: reloadTimer
        interval: 500
        repeat: false
        onTriggered: root.reloadLiveExample(codeEditor.text)
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_F2 && !standalone) {
            fullscreenStage = !fullscreenStage;
            event.accepted = true;
            return;
        }
        if (showingDemo) {
            if (event.key === Qt.Key_F5) {
                resetCurrentCode();
                event.accepted = true;
                return;
            }
            if (event.key === Qt.Key_F6 && !standalone) {
                if (codeEditor.activeFocus)
                    focusExample(liveExampleItem);
                else
                    codeEditor.focus = true;
                event.accepted = true;
                return;
            }
            if (event.key === Qt.Key_Escape || (event.key === Qt.Key_Backspace && !standalone && !codeEditor.activeFocus)) {
                closeDemo();
                event.accepted = true;
            } else if (event.key === Qt.Key_Q && !codeEditor.activeFocus) {
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
        case Qt.Key_Tab:
        case Qt.Key_BracketRight:
            categoryTabs.currentIndex = (categoryTabs.currentIndex + 1) % categoryNames.length;
            break;
        case Qt.Key_Backtab:
        case Qt.Key_BracketLeft:
            categoryTabs.currentIndex = (categoryTabs.currentIndex - 1 + categoryNames.length) % categoryNames.length;
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
            } else if (categoryTabs.currentIndex !== 0) {
                categoryTabs.currentIndex = 0;
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
        height: 2
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
                    color: "#58a6ff"
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
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "navigate  ",
                    color: root.mutedColor
                },
                {
                    text: " Enter/Space ",
                    color: root.brandPrimary,
                    bold: true
                },
                {
                    text: "open example  ",
                    color: root.mutedColor
                },
                {
                    text: " Tab ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "topic  ",
                    color: root.mutedColor
                },
                {
                    text: " / ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "search  ",
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
                    text: " F6 ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "focus code/preview  ",
                    color: root.mutedColor
                },
                {
                    text: " F5 ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "reset code  ",
                    color: root.mutedColor
                },
                {
                    text: " F2 ",
                    color: root.textColor,
                    bold: true
                },
                {
                    text: "toggle code pane  ",
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

    // Main Content Area
    Item {
        id: bodyArea
        anchors {
            top: root.standalone ? parent.top : header.bottom
            bottom: root.standalone ? parent.bottom : footer.top
            left: parent.left
            right: parent.right
        }

        // Screen 1: Full-width Component GridView Panel with topic TabBar & inline header search
        Panel {
            id: catalogPanel
            anchors {
                fill: parent
                leftMargin: 1
                rightMargin: 1
            }
            visible: !root.showingDemo
            color: root.tileColor
            border {
                style: Border.Rounded
                color: root.brandPrimary
            }
            title: Row {
                spacing: 1

                Text {
                    text: root.selectedCategory === "All" ? "Components" : "Components · " + root.selectedCategory
                    color: root.brandPrimary
                    bold: true
                }

                Text {
                    text: "(" + root.filteredCatalog.length + (root.searchQuery !== "" || root.selectedCategory !== "All" ? "/" + root.catalog.length : "") + ")"
                    color: root.mutedColor
                }
            }
            titleTrailing: Item {
                id: searchHeaderSlot
                implicitWidth: root.searchActive ? 26 : searchTriggerLabel.implicitWidth
                width: implicitWidth
                height: 1

                // Inactive compact trigger shown on the Panel top border
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

                // Active inline search TextInput embedded directly in the Panel border
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
            footer: Text {
                text: root.current ? "Selected: " + root.current.name + " · examples/" + root.current.file : "No matching components"
                color: root.mutedColor
            }
            footerTrailing: Text {
                text: "Enter ▸ Open Example & Code"
                color: root.brandPrimary
            }

            TabBar {
                id: categoryTabs
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    topMargin: 1
                    leftMargin: 2
                    rightMargin: 2
                }
                height: 1
                focusPolicy: Qt.NoFocus
                tabs: root.categoryNames
                color: root.tileColor
                textColor: root.mutedColor
                selectionColor: root.brandPrimary
                selectedTextColor: "#0d1117"

                onCurrentIndexChanged: {
                    root.currentIndex = 0;
                    root.ensureVisible();
                }
            }

            GridView {
                id: componentGrid
                anchors {
                    top: categoryTabs.bottom
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                    leftMargin: 2
                    rightMargin: 1
                    topMargin: 0
                    bottomMargin: 1
                }
                clip: true
                cellWidth: Math.max(26, Math.floor(width / root.gridCols))
                cellHeight: 5
                model: root.filteredCatalog

                delegate: Item {
                    id: tileItem
                    required property int index
                    required property var modelData
                    readonly property bool isCurrent: index === root.currentIndex

                    width: componentGrid.cellWidth
                    height: componentGrid.cellHeight

                    Rectangle {
                        id: tileCard
                        width: Math.max(10, parent.width - 1)
                        height: parent.height
                        clip: true
                        color: tileItem.isCurrent ? root.tileCurrentColor : root.tileColor
                        border {
                            style: tileItem.isCurrent ? Border.Double : Border.Rounded
                            color: tileItem.isCurrent ? tileItem.modelData.accent : root.lineColor
                        }

                        // Row 1: Component Name + Module Badge
                        Text {
                            x: 2
                            y: 1
                            text: (tileItem.isCurrent ? "▸ " : "") + tileItem.modelData.name
                            color: tileItem.isCurrent ? tileItem.modelData.accent : root.textColor
                            bold: true
                        }

                        Text {
                            anchors {
                                right: parent.right
                                rightMargin: 2
                            }
                            y: 1
                            text: "[" + tileItem.modelData.badge + "]"
                            color: tileItem.isCurrent ? root.brandPrimary : root.mutedColor
                        }

                        // Row 2: Representative Component Preview Content
                        Text {
                            x: 2
                            y: 2
                            width: Math.max(0, parent.width - 4)
                            clip: true
                            text: tileItem.modelData.preview
                            color: tileItem.modelData.accent
                            bold: tileItem.isCurrent
                        }

                        // Row 3: Concise Description Blurb
                        Text {
                            x: 2
                            y: 3
                            width: Math.max(0, parent.width - 4)
                            clip: true
                            text: tileItem.modelData.blurb
                            color: root.mutedColor
                        }

                        MouseArea {
                            anchors.fill: parent
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

        // Screen 2: Split View — Gallery-Owned Example Container (Left) + Editable Code Panel (Right)
        RowLayout {
            id: splitStage
            anchors {
                fill: parent
                leftMargin: 1
                rightMargin: 1
            }
            visible: root.showingDemo
            spacing: 1

            // Left Pane: Gallery-Owned Example Container Panel
            MouseArea {
                id: examplePaneArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: root.fullscreenStage || root.standalone ? parent.width : Math.max(34, Math.floor(parent.width * 0.50))
                onPressed: {
                    if (codeEditor.activeFocus || !root.exampleHasActiveFocus())
                        root.focusExample(root.liveExampleItem);
                }

                Panel {
                    id: examplePanel
                    anchors.fill: parent
                    color: root.tileColor
                    border {
                        style: !codeEditor.activeFocus ? Border.Double : Border.Rounded
                        color: root.current ? root.current.accent : root.brandPrimary
                    }
                    title: Row {
                        spacing: 1

                        Text {
                            text: root.current ? root.current.name : "Example"
                            color: root.current ? root.current.accent : root.brandPrimary
                            bold: true
                        }

                        Text {
                            visible: root.current !== null
                            text: root.current ? "[" + root.current.badge + "]" : ""
                            color: root.brandPrimary
                        }
                    }
                    titleTrailing: Text {
                        text: root.codeError !== "" ? "✗ Error" : (reloadTimer.running ? "⟳ Pending" : "● Live")
                        color: root.codeError !== "" ? "#f87171" : (reloadTimer.running ? "#f9c74f" : root.brandPrimary)
                        bold: true
                    }
                    footer: Text {
                        text: root.current ? "examples/" + root.current.file : ""
                        color: root.mutedColor
                    }

                    Item {
                        id: demoHost
                        anchors {
                            top: parent.top
                            left: parent.left
                            right: parent.right
                            bottom: errorBanner.visible ? errorBanner.top : parent.bottom
                            leftMargin: 2
                            rightMargin: 2
                            topMargin: 1
                            bottomMargin: 1
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
                        border {
                            style: Border.Single
                            color: "#f87171"
                        }

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

            // Right Pane: Live Editable QML Code Panel
            MouseArea {
                id: codePaneArea
                visible: !root.fullscreenStage && !root.standalone
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: Math.max(34, Math.floor(parent.width * 0.50))
                onPressed: {
                    codeEditor.focus = true;
                }

                Panel {
                    id: codePanel
                    anchors.fill: parent
                    color: root.tileColor
                    border {
                        style: codeEditor.activeFocus ? Border.Double : Border.Rounded
                        color: codeEditor.activeFocus ? root.brandPrimary : root.lineColor
                    }
                    title: Row {
                        spacing: 1

                        Text {
                            text: root.current ? "examples/" + root.current.file : "Code"
                            color: codeEditor.activeFocus ? root.brandPrimary : root.textColor
                            bold: true
                        }

                        Text {
                            text: "(editable)"
                            color: root.mutedColor
                        }
                    }
                    titleTrailing: Text {
                        text: reloadTimer.running ? "⟳ Reloading" : "✓ Auto-reload"
                        color: reloadTimer.running ? "#f9c74f" : root.brandPrimary
                    }
                    footer: Text {
                        text: "Ln " + (root.editorCursorLine + 1) + ", Col " + (root.editorCursorCol + 1) + " · 500ms auto-reload"
                        color: root.mutedColor
                    }
                    footerTrailing: Text {
                        text: "[F5 Reset]"
                        color: root.brandPrimary

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.resetCurrentCode()
                        }
                    }

                    // Active cursor line background highlight
                    Rectangle {
                        id: activeLineHighlight
                        x: 1
                        y: 1 + (root.editorCursorLine - codeEditor.contentY)
                        width: Math.max(0, parent.width - 2)
                        height: 1
                        visible: y >= 1 && y < parent.height - 1
                        color: codeEditor.activeFocus ? "#1f2d26" : "#1c232d"
                    }

                    // Line-number gutter (1 │, 2 │, …)
                    Column {
                        id: lineGutter
                        x: 1
                        y: 1
                        width: root.gutterWidth
                        height: Math.max(0, parent.height - 2)
                        clip: true

                        Repeater {
                            model: Math.max(0, lineGutter.height)

                            Text {
                                id: gutterRow
                                required property int index
                                readonly property int lineIndex: codeEditor.contentY + index
                                readonly property bool inBounds: lineIndex < root.editorLineCount
                                readonly property bool isCurrentLine: inBounds && lineIndex === root.editorCursorLine

                                width: root.gutterWidth
                                height: 1
                                spans: [
                                    {
                                        text: gutterRow.inBounds ? String(gutterRow.lineIndex + 1).padStart(root.gutterDigits, " ") : " ".repeat(root.gutterDigits - 1) + "~",
                                        color: gutterRow.isCurrentLine ? root.brandPrimary : root.mutedColor,
                                        bold: gutterRow.isCurrentLine,
                                        dim: !gutterRow.isCurrentLine
                                    },
                                    {
                                        text: " │",
                                        color: gutterRow.isCurrentLine ? root.brandPrimary : root.lineColor
                                    }
                                ]
                            }
                        }
                    }

                    TextArea {
                        id: codeEditor
                        anchors {
                            fill: parent
                            leftMargin: 1 + root.gutterWidth + 1
                            rightMargin: 1
                            topMargin: 1
                            bottomMargin: 1
                        }
                        color: root.textColor
                        showWhitespace: true

                        onTextChanged: {
                            if (!root.loadingExampleSource && root.showingDemo) {
                                root.applyQmlSyntaxHighlight();
                                reloadTimer.restart();
                            }
                        }
                    }
                }
            }
        }
    }
}
