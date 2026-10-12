pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// QmlTui Gallery: a full-screen grid of component tiles. Arrow keys (or hjkl)
// move the selection, Enter or Space opens the selected component's demo,
// and a mouse click opens a tile directly. Escape leaves a demo; in the grid
// Escape or Q quits.
//
// Every demo is a complete standalone app under examples/, each with its own
// imports and Terminal root. This launcher receives their text through the
// qmltuiExampleSources context property (main.cpp reads the resources, the web
// page passes what its editor holds), swaps the root Terminal for an Item, and
// instantiates the result inside the demo panel. The web page shows the same
// text in its editor, so what is read is what runs.
Terminal {
    id: root
    // The title names the open example so a host can follow along: the wasm
    // page listens to xterm's title event and puts that file into its editor.
    title: showingDemo && current ? "QmlTui Gallery · " + current.file : "QmlTui Gallery"
    color: "#0b0f14"

    // Palette
    readonly property string panelColor: "#0f151c"
    readonly property string tileColor: "#111821"
    readonly property string tileCurrentColor: "#182330"
    readonly property string lineColor: "#2a3542"
    readonly property string textColor: "#c9d1d9"
    readonly property string mutedColor: "#6e7b8b"

    // Grid state
    property int currentIndex: 0
    property int openIndex: -1
    property int scrollRow: 0
    readonly property bool showingDemo: openIndex >= 0
    readonly property bool inlineMode: mode === Terminal.Inline
    readonly property var current: showingDemo ? catalog[openIndex] : null

    readonly property int tileWidth: 28
    readonly property int tileHeight: 5
    readonly property int gap: 1
    readonly property int columns: Math.max(1, Math.floor((grid.width + gap) / (tileWidth + gap)))
    readonly property int visibleRows: Math.max(1, Math.floor((grid.height + gap) / (tileHeight + gap)))
    readonly property int totalRows: Math.ceil(catalog.length / columns)
    readonly property int gridOffsetX: Math.max(0, Math.floor((grid.width - (columns * (tileWidth + gap) - gap)) / 2))

    // The example files: { "Button.qml": "<source>", ... } as a JSON string.
    readonly property var sources: typeof qmltuiExampleSources === "string" ? JSON.parse(qmltuiExampleSources) : ({})
    // A host that edits the code itself (the wasm page) sets qmltuiGalleryPage:
    // the file to open at startup, "" for the grid.
    readonly property bool hostedByEditor: typeof qmltuiGalleryPage === "string"

    // The running demo, created from its file's text, and any error doing so.
    property Item demoItem: null
    property string demoError: ""

    // One entry per tile: name, module badge, a one-line blurb, a glyph
    // preview drawn in the accent colour, and the example file. Only entries
    // whose file the host provided are shown, so a build without Behavior or
    // QmlTui.Image simply has fewer tiles.
    readonly property var catalog: allEntries.filter(entry => entry.file in sources)
    readonly property var allEntries: [
        { name: "Button", badge: "Controls", accent: "#65e6a0", preview: "[ Primary ]  [ Cancel ]",
          blurb: "Enter, Space, click or hover", file: "Button.qml" },
        { name: "CheckBox", badge: "Controls", accent: "#7ee0ff", preview: "[x] Logging  [ ] Verbose",
          blurb: "Toggle with Space", file: "CheckBox.qml" },
        { name: "RadioButton", badge: "Controls", accent: "#f9c74f", preview: "(•) Fast  ( ) Safe",
          blurb: "One choice per group", file: "RadioButton.qml" },
        { name: "Switch", badge: "Controls", accent: "#ff9f6e", preview: "[ ON] Wi-Fi  [OFF] BT",
          blurb: "On/off with a label", file: "Switch.qml" },
        { name: "Slider", badge: "Controls", accent: "#c084fc", preview: "───●──────── [42]",
          blurb: "Left/Right or click", file: "Slider.qml" },
        { name: "ProgressBar", badge: "Controls", accent: "#38bdf8", preview: "▰▰▰▰▰▱▱▱▱▱  57%",
          blurb: "Styles and gradients", file: "ProgressBar.qml" },
        { name: "BusyIndicator", badge: "Controls", accent: "#f472b6", preview: "⠋ working  |  [ =  ]",
          blurb: "Five spinner styles", file: "BusyIndicator.qml" },
        { name: "TabBar", badge: "Controls", accent: "#fbbf24", preview: "│ Overview │ Logs │",
          blurb: "Tabs over a StackLayout", file: "TabBar.qml" },
        { name: "Panel", badge: "Controls", accent: "#94a3b8", preview: "┌ Title ────────┐",
          blurb: "Bordered box with title", file: "Panel.qml" },
        { name: "Dialog", badge: "Controls", accent: "#f87171", preview: "Delete?  [OK] [Cancel]",
          blurb: "Modal accept/reject", file: "Dialog.qml" },
        { name: "Popup", badge: "Controls", accent: "#a3e635", preview: "╔═ overlay ═╗",
          blurb: "Free-form modal overlay", file: "Popup.qml" },
        { name: "Toast", badge: "Controls", accent: "#34d399", preview: "┌ ✓ Saved ────┐",
          blurb: "Timed notifications", file: "Toast.qml" },
        { name: "Sparkline", badge: "Controls", accent: "#22d3ee", preview: "▁▂▃▅▇▆▄▃▂▁▂▄▆",
          blurb: "Live series in one row", file: "Sparkline.qml" },
        { name: "StackView", badge: "Controls", accent: "#e879f9", preview: "page 1 ▸ page 2 ▸ …",
          blurb: "Push and pop pages", file: "StackView.qml" },
        { name: "ToolTip", badge: "Controls", accent: "#fda4af", preview: "[ Build ] ┌ compiles… ┐",
          blurb: "Attached hover/focus tip", file: "ToolTip.qml" },
        { name: "StandardItemDelegate", badge: "Controls", accent: "#93c5fd", preview: "▸ [x] Build  [ ] Publish",
          blurb: "Default row for any view", file: "StandardItemDelegate.qml" },
        { name: "Text", badge: "Core", accent: "#f8fafc", preview: "bold  dim  spans  ▒▓█",
          blurb: "Styles, spans, gradients", file: "Text.qml" },
        { name: "Rectangle", badge: "Core", accent: "#60a5fa", preview: "┌─┐ ╔═╗ ┏━┓ +-+",
          blurb: "Borders and gradients", file: "Rectangle.qml" },
        { name: "RichText", badge: "Core", accent: "#fde68a", preview: "Wrapped rich text…",
          blurb: "Wrap, rich text, links", file: "RichText.qml" },
        { name: "TextInput", badge: "Core", accent: "#4ade80", preview: "Name: Ada█   Pw: •••",
          blurb: "Single-line editing", file: "TextInput.qml" },
        { name: "TextArea", badge: "Core", accent: "#a78bfa", preview: "Multi-line notes ⏎",
          blurb: "Multi-line editing", file: "TextArea.qml" },
        { name: "TableView", badge: "Core", accent: "#fb923c", preview: "│ PID │ NAME │ CPU │",
          blurb: "Columns and cell colors", file: "TableView.qml" },
        { name: "TreeView", badge: "Core", accent: "#86efac", preview: "▾ src  ▸ items  📄 a.cpp",
          blurb: "Expandable node tree", file: "TreeView.qml" },
        { name: "ListView", badge: "Core", accent: "#f472b6", preview: "› Alice   Bob   Carol",
          blurb: "Delegates over a model", file: "ListView.qml" },
        { name: "Image", badge: "Image", accent: "#f9a8d4", preview: "▀▄▀▄ ⣿⣷⣄ .:-=+*#",
          blurb: "Pixels as cells: half-block, Braille, ASCII", file: "Image.qml" },
        { name: "AnimatedSprite", badge: "Image", accent: "#fdba74", preview: "▶ ▄▀▄ ▄█▄ ▄▀▄  1/4",
          blurb: "Frames out of one sheet", file: "AnimatedSprite.qml" },
        { name: "Canvas", badge: "Core", accent: "#00f0ff", preview: "⢀⡠⠔⠒⠑⠢⢄⡀ ⣾⣿⣷",
          blurb: "Animated Braille shapes", file: "Canvas.qml" },
        { name: "Layouts", badge: "Core", accent: "#fcd34d", preview: "[■■][■■■■■■][■■]",
          blurb: "Row, Column, GridLayout", file: "Layouts.qml" },
        { name: "GridView", badge: "Core", accent: "#fbbf24", preview: "▦ ▦ ▦ ▦   ▦ ▦ ▦ ▦",
          blurb: "Cells over a model", file: "GridView.qml" },
        { name: "ScrollView", badge: "Core", accent: "#67e8f9", preview: "line 12 ▐ … line 30 ▌",
          blurb: "Scroll any tall content", file: "ScrollView.qml" },
        { name: "LogView", badge: "Core", accent: "#a3e635", preview: "12:00:04 INFO ready ▼",
          blurb: "Append-only tail log", file: "LogView.qml" },
        { name: "Loader", badge: "Core", accent: "#fcd34d", preview: "sourceComponent: pageA",
          blurb: "Create, swap on demand", file: "Loader.qml" },
        { name: "Repeater", badge: "Core", accent: "#c4b5fd", preview: "▮▮▮▮▮▮▮  ★ ★ ★",
          blurb: "One delegate per entry", file: "Repeater.qml" },
        { name: "MouseArea", badge: "Core", accent: "#f97316", preview: "click ▪ hover (12, 3)",
          blurb: "Clicks, presses, hover", file: "MouseArea.qml" },
        { name: "Shortcut", badge: "Core", accent: "#fb7185", preview: "1 2 3 ▸ theme",
          blurb: "Global key without focus", file: "Shortcut.qml" },
        { name: "States", badge: "Core", accent: "#34d399", preview: "OFF ──▶ ON  (300 ms)",
          blurb: "State + Transition", file: "States.qml" },
        { name: "Animation", badge: "Core", accent: "#f9a8d4", preview: "○───────▶●",
          blurb: "Number and Color, easing", file: "Animation.qml" },
        { name: "Behavior", badge: "Core", accent: "#c084fc", preview: "x: 0 ─▶ 12  (animated)",
          blurb: "Animate every change of a property", file: "Behavior.qml" },
        { name: "Inline mode", badge: "Terminal", accent: "#38bdf8", preview: "❯ prompt   ▲ scrollback",
          blurb: "Inline + Scrollback", file: "InlineMode.qml" }
    ]

    // Turns a standalone example (imports, `Terminal { ... }`) into a document
    // whose root is an Item that fills its parent, so it can live inside the
    // demo panel. Root-level properties that only a Terminal has (title,
    // color, mode, the quit handler) are dropped; `mode: Terminal.Inline`
    // is reported so the launcher can switch its own mode. Brace matching
    // skips strings and // comments; a dropped statement may span lines.
    function embeddable(source) {
        const lines = source.split("\n")
        const rootLine = lines.findIndex(line => /^Terminal\s*\{\s*$/.test(line))
        if (rootLine < 0)
            throw new Error("the root object must be a Terminal")
        const out = lines.slice(0, rootLine)
        out.push("Item {")
        let inline = false
        let depth = 0
        let quote = ""
        let dropping = false
        for (let i = rootLine; i < lines.length; ++i) {
            const line = lines[i]
            const depthBefore = depth
            for (let c = 0; c < line.length; ++c) {
                const ch = line[c]
                if (quote !== "") {
                    if (ch === "\\") c++
                    else if (ch === quote) quote = ""
                    continue
                }
                if (ch === "/" && line[c + 1] === "/")
                    break
                if (ch === "\"" || ch === "'" || ch === "`") quote = ch
                else if (ch === "{") depth++
                else if (ch === "}") depth--
            }
            if (i === rootLine)
                continue
            if (dropping) {
                if (depth <= 1)
                    dropping = false
                continue
            }
            if (depthBefore === 1) {
                const m = line.match(/^\s*(title|color|mode|mouseReporting|clipboardMode|Keys\.\w+)\s*:/)
                if (m) {
                    if (m[1] === "mode" && line.includes("Inline"))
                        inline = true
                    dropping = depth > 1
                    continue
                }
            }
            out.push(line)
        }
        out.splice(rootLine + 1, 0, inline ? "    width: parent.width" : "    anchors.fill: parent")
        return { text: out.join("\n"), inline: inline }
    }

    function ensureVisible() {
        const row = Math.floor(currentIndex / columns)
        let first = scrollRow
        if (row < first)
            first = row
        else if (row >= first + visibleRows)
            first = row - visibleRows + 1
        first = Math.max(0, Math.min(first, totalRows - visibleRows))
        scrollRow = first
    }

    function move(delta) {
        const next = currentIndex + delta
        if (next < 0 || next >= catalog.length)
            return
        currentIndex = next
    }

    function openCurrent() {
        closeDemoItem()
        openIndex = currentIndex
        const entry = catalog[currentIndex]
        const source = sources[entry.file]
        if (source === undefined) {
            demoError = entry.file + " was not provided"
            return
        }
        let built
        try {
            built = embeddable(source)
        } catch (e) {
            demoError = String(e.message || e)
            return
        }
        const host = built.inline ? inlineHost : demoHost
        try {
            // The path only names the document for errors; it must not point
            // into a real directory or its files would be imported implicitly.
            demoItem = Qt.createQmlObject(built.text, host, "gallery-example/" + entry.file)
        } catch (e) {
            const errors = e.qmlErrors || []
            demoError = errors.length
                ? errors.map(err => entry.file + ":" + err.lineNumber + ":" + err.columnNumber + ": " + err.message).join("\n")
                : String(e.message || e)
            return
        }
        if (built.inline) {
            // An Item has no content height of its own; measure the example once.
            let height = 0
            for (let i = 0; i < demoItem.children.length; ++i) {
                const child = demoItem.children[i]
                height = Math.max(height, child.y + (child.height > 0 ? child.height : child.implicitHeight))
            }
            demoItem.height = height
            mode = Terminal.Inline
        }
        focusExample(demoItem)
    }

    // `focus: true` inside a document created while detached is not recorded,
    // so give focus to the item the example marks, or else to its first
    // focusable control, once it is attached.
    function focusExample(item) {
        let fallback = null
        const marked = (function visit(node) {
            for (let i = 0; i < node.children.length; ++i) {
                const child = node.children[i]
                if (child.focus)
                    return child
                if (!fallback && child.focusable && child.visible && child.enabled)
                    fallback = child
                const found = visit(child)
                if (found)
                    return found
            }
            return null
        })(item)
        const target = marked || fallback
        if (target)
            target.focus = true
    }

    function closeDemoItem() {
        demoError = ""
        if (demoItem) {
            // Detach first: destroy() deletes at the end of the event loop
            // cycle, and the item must not paint or take keys until then.
            const item = demoItem
            demoItem = null
            item.visible = false
            item.parent = null
            item.destroy()
        }
    }

    function closeDemo() {
        closeDemoItem()
        openIndex = -1
        mode = Terminal.AltScreen
        root.focus = true
    }

    Component.onCompleted: {
        if (!hostedByEditor)
            return
        const index = catalog.findIndex(entry => entry.file === qmltuiGalleryPage)
        if (index >= 0) {
            currentIndex = index
            openCurrent()
        }
    }

    onCurrentIndexChanged: ensureVisible()
    onColumnsChanged: ensureVisible()
    onVisibleRowsChanged: ensureVisible()

    Keys.onPressed: event => {
        if (showingDemo) {
            if (event.key === Qt.Key_Escape || event.key === Qt.Key_Backspace) {
                closeDemo()
                event.accepted = true
            } else if (event.key === Qt.Key_Q) {
                Qt.quit()
                event.accepted = true
            }
            return
        }
        switch (event.key) {
        case Qt.Key_Left: case Qt.Key_H: move(-1); break
        case Qt.Key_Right: case Qt.Key_L: move(1); break
        case Qt.Key_Up: case Qt.Key_K: move(-columns); break
        case Qt.Key_Down: case Qt.Key_J: move(columns); break
        case Qt.Key_PageUp: move(-columns * visibleRows); break
        case Qt.Key_PageDown: move(columns * visibleRows); break
        case Qt.Key_Home: currentIndex = 0; break
        case Qt.Key_End: currentIndex = catalog.length - 1; break
        case Qt.Key_Return: case Qt.Key_Enter: case Qt.Key_Space: openCurrent(); break
        case Qt.Key_Q: case Qt.Key_Escape: Qt.quit(); break
        default: return
        }
        event.accepted = true
    }

    // Header
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        visible: !root.inlineMode
        color: root.panelColor

        Text {
            x: 2; y: 0
            visible: !root.showingDemo
            text: "QmlTui Gallery"
            bold: true
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#7ee0ff" }
                GradientStop { position: 1.0; color: "#c084fc" }
            }
        }
        Text {
            y: 1
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.right: scrollInfo.left
            anchors.rightMargin: 2
            visible: !root.showingDemo
            text: root.catalog.length + " components from QmlTui and QmlTui.Controls, rendered in text cells"
            color: root.mutedColor
        }
        Text {
            x: 2; y: 0
            visible: root.showingDemo
            spans: [
                { text: "◂ Esc", color: root.mutedColor },
                { text: "  " + (root.current ? root.current.name : ""), color: root.current ? root.current.accent : root.textColor, bold: true },
                { text: "  " + (root.current ? root.current.badge : ""), color: root.mutedColor }
            ]
        }
        Text {
            x: 2; y: 1
            visible: root.showingDemo
            text: root.current ? root.current.blurb : ""
            color: root.mutedColor
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 2
            y: 0
            visible: !root.showingDemo
            text: (root.currentIndex + 1) + " / " + root.catalog.length
            color: root.mutedColor
        }
        Text {
            id: scrollInfo
            anchors.right: parent.right
            anchors.rightMargin: 2
            y: 1
            visible: !root.showingDemo && root.totalRows > root.visibleRows
            text: "rows " + (root.scrollRow + 1) + "–" + Math.min(root.totalRows, root.scrollRow + root.visibleRows) + " of " + root.totalRows + " ▴▾"
            color: root.mutedColor
        }
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: root.lineColor
        }
    }

    // Footer
    Rectangle {
        id: footer
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        visible: !root.inlineMode
        color: root.panelColor

        Text {
            x: 2
            visible: !root.showingDemo
            spans: [
                { text: " ↑↓←→ ", color: root.textColor, bold: true }, { text: "move  ", color: root.mutedColor },
                { text: " Enter ", color: root.textColor, bold: true }, { text: "open  ", color: root.mutedColor },
                { text: " click ", color: root.textColor, bold: true }, { text: "open  ", color: root.mutedColor },
                { text: " Q ", color: root.textColor, bold: true }, { text: "quit", color: root.mutedColor }
            ]
        }
        Text {
            x: 2
            visible: root.showingDemo
            spans: [
                { text: " Esc ", color: root.textColor, bold: true }, { text: "back to grid  ", color: root.mutedColor },
                { text: " Tab ", color: root.textColor, bold: true }, { text: "next control  ", color: root.mutedColor },
                { text: " Q ", color: root.textColor, bold: true }, { text: "quit", color: root.mutedColor }
            ]
        }
    }

    // Grid of tiles
    Item {
        id: grid
        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 1
        clip: true
        visible: !root.showingDemo && !root.inlineMode

        Repeater {
            model: root.catalog
            delegate: Rectangle {
                id: tile
                required property int index
                required property var modelData
                readonly property int column: index % root.columns
                readonly property int row: Math.floor(index / root.columns)
                readonly property bool current: index === root.currentIndex

                visible: row >= root.scrollRow && row < root.scrollRow + root.visibleRows
                x: root.gridOffsetX + column * (root.tileWidth + root.gap)
                y: (row - root.scrollRow) * (root.tileHeight + root.gap)
                width: root.tileWidth
                height: root.tileHeight
                clip: true
                color: current ? root.tileCurrentColor : root.tileColor
                border.style: current ? Border.Double : Border.Single
                border.color: current ? modelData.accent : root.lineColor

                Text {
                    x: 2; y: 1
                    text: tile.modelData.name
                    bold: true
                    color: tile.current ? tile.modelData.accent : root.textColor
                }
                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    y: 1
                    // Long names such as StandardItemDelegate keep the row; the badge yields.
                    visible: tile.modelData.name.length + text.length + 6 <= tile.width
                    text: tile.modelData.badge
                    color: root.mutedColor
                    dim: true
                }
                Text {
                    x: 2; y: 2
                    width: tile.width - 4
                    text: tile.modelData.preview
                    color: tile.modelData.accent
                }
                Text {
                    x: 2; y: 3
                    width: tile.width - 4
                    text: tile.modelData.blurb
                    color: tile.current ? root.textColor : root.mutedColor
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.currentIndex = tile.index
                        root.openCurrent()
                    }
                }
            }
        }

    }

    // Demo view: the open example, instantiated from its file.
    Item {
        id: demoView
        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 1
        visible: root.showingDemo && !root.inlineMode

        Panel {
            anchors.fill: parent
            title: root.current ? root.current.name + "  ·  examples/" + root.current.file : ""
            titleColor: root.current ? root.current.accent : root.textColor
            border.style: Border.Single
            border.color: root.lineColor
            color: root.panelColor

            Item {
                id: demoHost
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                anchors.rightMargin: 2
                clip: true   // tall examples stop at the panel border
            }

            Text {
                anchors.fill: parent
                anchors.margins: 2
                visible: root.demoError !== ""
                wrapMode: Text.WordWrap
                color: "#f87171"
                text: root.demoError
            }
        }
    }

    // Inline examples are created here; the launcher switches its own mode
    // and hides its full-screen chrome, so only this item contributes height.
    Item {
        id: inlineHost
        visible: root.inlineMode
        width: parent.width
        height: root.inlineMode && root.demoItem ? root.demoItem.height : 0
    }
}
