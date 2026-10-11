# 02: Layout and text

The Rectangle fills the terminal and draws a one-cell border. Anchors inset
its Column; the Column then positions its own children. RowLayout shares the
available width, while Row places content at its natural width. Paragraph wraps
when resized. Text spans apply styles to individual literal text runs.

Build with `cmake --build build --target qamui_tutorial_02_layout_and_text` and run
`./build/examples/tutorials/02_layout_and_text/qamui_tutorial_02_layout_and_text`. Press Escape to quit.

<!-- include: examples/tutorials/02_layout_and_text/Main.qml -->
```qml
pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Controls

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
        border.style: "single"
        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1
            RowLayout {
                width: parent.width
                Text { text: "Layout and text"; Layout.fillWidth: true }
                Text { text: "Esc quits" }
            }
            Paragraph {
                width: parent.width
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

<!-- include: examples/tutorials/02_layout_and_text/test_tutorial.js -->
```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Layout and text");
    terminal.resize(60, 20);
    terminal.expectText("source text.");
    terminal.expectText("Ready to build");
    terminal.captureText("tutorial.txt");
}
```

[Tutorial index](index.md)
