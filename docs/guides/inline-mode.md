# Inline Mode and Scrollback

By default, `Terminal.mode` is `Terminal.AltScreen`, which switches to the terminal's alternate screen buffer (`\033[?1049h`) and fills the full viewport. Setting `mode: Terminal.Inline` keeps your application in the normal terminal buffer: it renders a live interactive region at the bottom of the terminal while streaming permanent output into the native terminal scrollback above it via [`Scrollback`](../qml-types/scrollback.md).

---

## 1. Minimal Inline REPL Example

In `Terminal.Inline` mode, the root `Terminal` sizes its height dynamically to its visible children (up to `terminalRows - 1`). Give your root layout a finite implicit or explicit height, and use `Scrollback` to commit completed lines above the live prompt:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: root
    mode: Terminal.Inline

    Scrollback {
        id: history
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            if (promptInput.text.length > 0) {
                promptInput.text = "";
            } else {
                Qt.quit();
            }
            event.accepted = true;
        }
    }

    Column {
        width: parent.width

        Rectangle {
            width: parent.width
            height: 3
            border {
                style: Border.Rounded
                color: "cyan"
            }

            TextInput {
                id: promptInput
                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                placeholderText: "Type a message (Enter commits to scrollback, Esc quits)..."
                Keys.onReturnPressed: event => {
                    if (promptInput.text.length > 0) {
                        history.print("> " + promptInput.text);
                        promptInput.text = "";
                    }
                    event.accepted = true;
                }
            }
        }
    }
}
```

---

## 2. `Scrollback` APIs: `print()`, `append()`, and `model` + `delegate`

`Scrollback` is a non-visual object with three ways to commit permanent history rows:

1. **`print(text)`**: Appends plain or newline-separated text lines directly above the live viewport.
2. **`append(itemOrComponent, properties)`**: Renders a QML `Item` or `Component` snapshot off-screen and commits its styled cells into scrollback history.
3. **`model` + `delegate`**: Binds an append-only `ListModel` or `QAbstractItemModel` so newly inserted rows are automatically instantiated, painted once, and committed to scrollback.

---

## 3. Inline Geometry, Mouse Origin (`CSI 6 n`), and Queue Limits

- **Finite Child Height**: Never set `anchors.fill: parent` or `height: parent.height` on the top-level child of an inline `Terminal`—size the live region from its content (`Column`, fixed `height`, etc.). Hidden children (`visible: false`) do not contribute to inline height.
- **Cursor Position Reporting (`CSI 6 n`)**: Because inline output starts at whatever row the shell cursor occupied when the process launched, `InlineRenderer` queries the cursor row with `CSI 6 n` (`\033[6n`) to translate terminal mouse coordinates to the live region's screen origin. While a cursor report is pending, mouse clicks do not activate items.
- **Bounded 1 MiB Queue**: Pending scrollback output is buffered in a 1 MiB session queue (and held while temporarily in `Terminal.AltScreen` mode). If a producer exceeds the limit, `print()` returns `false` and `Scrollback` emits `outputRejected(reason)`.

See the runnable guide example in [`examples/guides/inline_repl/`](../../examples/guides/inline_repl/), the gallery demo [`InlineMode.qml`](../../examples/gallery/examples/InlineMode.qml), and the [`Terminal`](../qml-types/terminal.md) and [`Scrollback`](../qml-types/scrollback.md) references.
