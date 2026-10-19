# Input and Focus

Terminal applications rely first on keyboard navigation. QmlTui maintains an explicit active focus chain, routes decoded keystrokes from the focused item up to the root `Terminal`, and supports SGR mouse tracking and bracketed paste.

---

## 1. Active Focus and Tab Chains

Set `focus: true` on the initial interactive item. Focusable items (`focusable: true`, enabled by default on interactive controls and `TextInput`/`TextArea`) participate in the **Tab** / **Shift+Tab** focus ring in tree order.

Always provide clear visual feedback for `activeFocus` using borders or text attributes:

```qml
Rectangle {
    width: 24
    height: 3
    focusable: true
    border {
        style: activeFocus ? Border.Double : Border.Single
        color: activeFocus ? "cyan" : "gray"
    }
}
```

---

## 2. Key Event Bubbling and Interception

Attached [`Keys`](../qml-types/keys.md) handlers (`Keys.onPressed: event => { ... }`, `Keys.onReturnPressed`, `Keys.onEscapePressed`, `Keys.onUpPressed`) fire on the item with `activeFocus`. If `event.accepted` is left `false`, the event bubbles up through parent items to the root `Terminal`.

- **`Keys.priority: Keys.BeforeItem`**: Intercepts keys before the item's built-in C++ handler runs (useful for overriding keys inside `TextInput` or `PtyView`).
- **`Keys.forwardTo: [targetItem]`**: Forwards unhandled key events from a container or search bar directly to another item such as a `ListView` or `TableView`.

---

## 3. Global `Shortcut` vs. Contextual `Escape` Quit Path

In terminal raw mode, `Ctrl+C` does **not** send `SIGINT` (allowing `TextInput` and `TextArea` to use `Ctrl+C` for clipboard copy), and the root `Terminal` never exits automatically on `Q` or `Escape`. Every application must define an explicit keyboard quit path:

- Never bind bare `"Q"` as a global [`Shortcut`](../qml-types/shortcut.md) when the scene contains a `TextInput` or `TextArea`, or typing `q` into the editor will quit the app. Use `Shortcut { key: "Ctrl+Q"; onActivated: Qt.quit() }` or handle `Qt.Key_Q` on the root `Terminal` after editable inputs have had a chance to consume it.
- Preserve contextual `Escape` order: close open [`Dialog`](../qml-types/dialog.md) / [`Popup`](../qml-types/popup.md) overlays first, clear active filter text second, and quit only when nothing else consumed `Escape`:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root

    Shortcut {
        key: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            if (filterInput.text.length > 0) {
                filterInput.text = "";
            } else {
                Qt.quit();
            }
            event.accepted = true;
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        TextInput {
            id: filterInput
            width: 28
            placeholderText: "Filter (Esc clears, then quits)..."
            focus: true
        }

        Button {
            text: "Open Modal Dialog"
            onClicked: confirmDialog.open()
        }
    }

    Dialog {
        id: confirmDialog
        title: "Confirm Action"
        message: "Press Esc to close this modal without quitting."
    }
}
```

---

## 4. Mouse Input (`MouseArea`)

[`MouseArea`](../qml-types/mousearea.md) translates SGR 1006 terminal mouse sequences into cell-coordinate `onClicked`, `onDoubleClicked`, `onPressed`, `onReleased`, `onPositionChanged` (drag), `onWheel`, and `containsMouse` (when `hoverEnabled: true`). Always pair mouse interactions with keyboard equivalents so the UI remains operable over any terminal session.

See the gallery examples [`Shortcut.qml`](../../examples/gallery/examples/Shortcut.qml), [`MouseArea.qml`](../../examples/gallery/examples/MouseArea.qml), and [`Dialog.qml`](../../examples/gallery/examples/Dialog.qml), plus the [`Keys`](../qml-types/keys.md), [`Shortcut`](../qml-types/shortcut.md), and [`MouseArea`](../qml-types/mousearea.md) references.
