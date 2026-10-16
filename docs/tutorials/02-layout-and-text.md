# 02: Layout and text

The Rectangle fills the terminal and draws a one-cell border. Anchors inset
its Column; the Column then positions its own children. RowLayout shares the
available width, while Row places content at its natural width. Text wraps
when resized if wrapMode is set. Text spans apply styles to individual literal text runs.

Build with `cmake --build build --target qmltui_tutorial_02_layout_and_text` and run
`./build/examples/tutorials/02_layout_and_text/qmltui_tutorial_02_layout_and_text`. Press Escape to quit.

## Terminal Output Preview

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│Layout and text                                                      Esc quits│
│                                                                              │
│                                                                              │
│A paragraph wraps at the available cell width. Resize the terminal to see     │
│these words move across lines without changing the source text.               │
│                                                                              │
│Ready to build  Integer cells                                                 │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

## Source Files

### `Main.qml`

<!-- include: examples/tutorials/02_layout_and_text/Main.qml -->
```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }
    Rectangle {
        anchors.fill: parent
        border.style: Border.Single
        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1
            RowLayout {
                width: parent.width
                Text { text: "Layout and text"; Layout.fillWidth: true }
                Text { text: "Esc quits" }
            }
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: "A paragraph wraps at the available cell width. Resize the terminal to see these words move across lines without changing the source text."
            }
            Row {
                spacing: 2
                Text { spans: [{ text: "Ready", bold: true }, { text: " to build", color: "cyan" }] }
                Text { text: "Integer cells" }
            }
        }
    }
}
```

## Key Concepts in This Chapter

- **Borders & Anchor Margins**: `Rectangle` with `border.style: Border.Single` draws a 1-cell box-drawing frame around the viewport. Setting `anchors.fill: parent` and `anchors.margins: 1` on the inner `Column` insets its content by one cell on every side so children never overwrite the border glyphs.
- **`RowLayout` vs. `Row` Positioners**: `RowLayout` distributes available width across its children—attaching `Layout.fillWidth: true` to the left `Text` pushes `"Esc quits"` flush against the right edge. By contrast, `Row` simply places items side by side at their natural `implicitWidth` separated by `spacing: 2`.
- **Responsive Word Wrapping (`Text.WordWrap`)**: Giving a `Text` item `width: parent.width` and `wrapMode: Text.WordWrap` reflows paragraphs automatically when the terminal resizes, updating `implicitHeight` so subsequent items in the `Column` shift down cleanly.
- **Rich-Text `spans`**: Instead of embedding raw ANSI escape codes or HTML tags, pass an array of `{ text, bold, color, ... }` objects to `Text.spans` to style individual runs within a single text item.

---

[← Previous: 01: First terminal](01-first-terminal.md) | [Tutorial index](index.md) | [Next: 03: Input and focus →](03-input-and-focus.md)
