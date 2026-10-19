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
                        history.print([
                            { text: "> ", color: "cyan", bold: true },
                            { text: promptInput.text }
                        ]);
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

### A. Fast Direct Output with `print()`
Bypasses delegates, spacing, and separators to send text or styled spans directly above the live viewport:

```qml
// Plain string:
scrollback.print("Simple log line");

// Single styled span literal:
scrollback.print({ text: "Build succeeded", color: "green", bold: true });

// Array of styled span literals:
scrollback.print([
    { text: "❯ ", color: "#38bdf8", bold: true },
    { text: "deploy --prod\n", color: "#f8fafc" },
    { text: "  ✓ done in 0.4s", color: "#2cde85" }
]);
```

### B. Declarative Streaming with `model` + `delegate`
Bind a `ListModel` or C++ `QAbstractItemModel`. As rows are inserted, `Scrollback` instantiates the delegate item off-screen, paints it once, and commits it into terminal history:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: root
    mode: Terminal.Inline

    ListModel {
        id: messageLog
    }

    Scrollback {
        id: history
        model: messageLog
        spacing: 0

        delegate: Row {
            id: rowRoot
            required property string sender
            required property string message
            required property string time
            spacing: 1

            Text {
                text: "[" + rowRoot.time + "]"
                color: "#8b949e"
            }
            Text {
                text: rowRoot.sender + ":"
                bold: true
                color: rowRoot.sender === "User" ? "#38bdf8" : "#2cde85"
            }
            Text {
                text: rowRoot.message
            }
        }
    }

    // Appending a row to the model commits it directly to scrollback:
    function addMessage(sender, message) {
        messageLog.append({
            sender: sender,
            message: message,
            time: new Date().toLocaleTimeString()
        });
    }
}
```

### C. Imperative Templating with `delegate` + `append(data)`
When you don't need a persistent `ListModel`, define a `delegate` component template on `Scrollback` and feed data objects directly via `append(data)`:

```qml
Scrollback {
    id: buildLog
    spacing: 1

    delegate: Column {
        id: entryDelegate
        required property string title
        required property string detail
        required property bool success

        Text {
            text: (entryDelegate.success ? "✓ " : "✗ ") + entryDelegate.title
            color: entryDelegate.success ? "#2cde85" : "#f87171"
            bold: true
        }
        Text {
            text: "  " + entryDelegate.detail
            color: "#8b949e"
        }
    }
}

// In JavaScript / signal handlers:
buildLog.append({
    title: "Compilation",
    detail: "Finished in 1.4s (42 targets)",
    success: true
});
```

### Key Considerations for Delegates
- **`required property` Declarations**: Under `pragma ComponentBehavior: Bound`, declare `required property <Type> <roleName>` on the delegate root for every model or data property.
- **Append-Only Contract**: `Scrollback` renders each entry once upon insertion/call and submits it immediately into the terminal emulator's native scrollback. Subsequent property or model modifications to previously emitted rows are not redrawn.
- **Pooling with `reuseItems`**: Enabled by default (`reuseItems: true`), which pools and reuses the delegate instance across entries to minimize allocations during high-volume output streaming.
- **`separator` and `spacing`**: You can set `spacing: N` for blank lines between entries, and provide a `separator: Component { ... }` (e.g. a dashed line or divider) rendered between successive items.

---

## 3. Inline Geometry, Mouse Origin (`CSI 6 n`), and Queue Limits

- **Finite Child Height**: Never set `anchors.fill: parent` or `height: parent.height` on the top-level child of an inline `Terminal`—size the live region from its content (`Column`, fixed `height`, etc.). Hidden children (`visible: false`) do not contribute to inline height.
- **Cursor Position Reporting (`CSI 6 n`)**: Because inline output starts at whatever row the shell cursor occupied when the process launched, `InlineRenderer` queries the cursor row with `CSI 6 n` (`\033[6n`) to translate terminal mouse coordinates to the live region's screen origin. While a cursor report is pending, mouse clicks do not activate items.
- **Bounded 1 MiB Queue**: Pending scrollback output is buffered in a 1 MiB session queue (and held while temporarily in `Terminal.AltScreen` mode). If a producer exceeds the limit, `print()` returns `false` and `Scrollback` emits `outputRejected(reason)`.

See the runnable recipe example in [`examples/recipes/inline_repl/`](../../examples/recipes/inline_repl/), the gallery demo [`InlineMode.qml`](../../examples/gallery/examples/InlineMode.qml), and the [`Terminal`](../qml-types/terminal.md) and [`Scrollback`](../qml-types/scrollback.md) references.
