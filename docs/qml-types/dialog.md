---
title: Dialog
kind: qml
import: QmlTui.Controls
since: "0.4"
inherits: Popup
cpp: qmltui::Popup
category: overlays and navigation
summary: A modal, centred message box with a title, a message and accept and reject buttons.
---

# Dialog QML type

A modal, centred message box with a title, a message and accept and reject buttons. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui.Controls` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Popup](popup.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `background` | `Rectangle` |  | Read-only. Holds the background rectangle composed behind the dialog content. Customize `background.color`, `background.border` and `background.gradient`. |
| `textColor` | `rgba` | `"default"` | Holds the terminal colour of the message text, where `"default"` is the terminal's own foreground colour. |
| `titleColor` | `rgba` |  | Holds the terminal colour of the bold title line; it follows `textColor` unless set explicitly. |
| `title` | `string` | `""` | Holds the bold heading drawn on the first content row of the dialog. |
| `message` | `string` | `""` | Holds the single-line body text drawn below the title. |
| `acceptText` | `string` | `"OK"` | Holds the label of the accept button, which calls `accept()` when activated. |
| `rejectText` | `string` | `"Cancel"` | Holds the label of the reject button, which calls `reject()` when activated. |
| `showRejectButton` | `bool` | `true` | Holds whether the reject button is shown; set it to false for a single-button notice. |

## Detailed Description

Dialog is a preconfigured [Popup](popup.md): it is `modal`, dims the
background, is anchored to the centre of its parent, and composes an opaque
[Rectangle](rectangle.md) accessible via `background` with a `Border.Double`
border and default terminal colors. Its `closePolicy` is `Popup.CloseOnEscape` only,
so Escape calls `reject()` but clicking outside the dialog does nothing.
Callers customize the visual surface through `background.color`,
`background.border`, and `background.gradient`.
Open it with `open()` and react to `accepted()`, `rejected()` or
`finished(bool)`, all of which are inherited from Popup and are followed by
`close()`.

The content is a [Column](column.md) at cell (2, 1) with one blank row between
the title, the message and a [Row](row.md) of two [Button](button.md)s. The
`initialFocusItem` is the accept button, so on `open()` focus moves there and
Enter or Space activates it; Tab and Shift+Tab cycle focus between the two
buttons only, because the popup is modal. Enter or Return that reaches the
dialog itself without being consumed by a child triggers the accept button.
Closing restores focus to the item that was focused before `open()`.

The implicit size is the content's implicit width plus 4 cells, but at least
32 cells wide, by the content height plus 2 rows; with a one-line title and
message that is 9 rows. The message is not wrapped, so a long `message`
widens the dialog rather than adding lines. Use [Popup](popup.md) directly
when you need custom content or a non-modal overlay, and
[Toast](toast.md) for notifications that need no answer.

### Example

```qml
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    Column {
        anchors.centerIn: parent
        spacing: 1

        Button {
            text: "Delete file"
            focus: true
            onClicked: confirm.open()
        }

        Text {
            id: status
            text: "Ready."
        }
    }

    Dialog {
        id: confirm
        title: "Confirm"
        message: "Delete the file? This cannot be undone."
        acceptText: "Delete"
        onAccepted: Qt.quit()
        onRejected: status.text = "Cancelled."
    }
}
```

### Terminal Preview

```text
╔═════════════╗
║ Delete file ║
╚═════════════╝

Ready.

╔═════════════════════════════════════════╗
║ Confirm                                 ║
║                                         ║
║ Delete the file? This cannot be undone. ║
║                                         ║
║ ╔══════════╗  ┌──────────┐              ║
║ ║  Delete  ║  │  Cancel  │              ║
║ ╚══════════╝  └──────────┘              ║
╚═════════════════════════════════════════╝
```

### Interactive Demo & Recording

![Dialog recorded demo](../assets/casts/components/dialog.gif)

- **Live Playground**: [▶ Open `Dialog.qml` in the WebAssembly Playground](../playground.md?example=Dialog.qml)
- **Standalone CLI**: `./build/examples/gallery/qmltui_gallery --example Dialog`
- **Gallery Source**: [`examples/gallery/examples/Dialog.qml`](../../examples/gallery/examples/Dialog.qml)
