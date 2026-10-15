---
title: AbstractButton
kind: qml
import: QmlTui.Controls
since: "0.4"
inherits: Item
cpp: qmltui::Item
category: inputs
summary: "The interaction template every button-like control is built on: text, pressed and down, checkable and checked, hover, paddings, and the background and contentItem items"
---

# AbstractButton QML type

The interaction template every button-like control is built on: text, `pressed` and `down`, `checkable` and `checked`, hover, paddings, and the `background` and `contentItem` items. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui.Controls` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |
| **Inherited By** | [Button](button.md), [CheckBox](checkbox.md), [RadioButton](radiobutton.md), [Switch](switch.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` | `""` | Holds the label the default `contentItem` of each control draws. |
| `pressed` | `bool` | `false` | Holds whether a mouse button is currently held down on the control; keyboard activation never sets it, since terminals report no key releases. |
| `down` | `bool` | `pressed` | Holds the visual pressed state used by styles; it follows `pressed` unless assigned explicitly. |
| `checkable` | `bool` | `false` | Whether activation toggles `checked` before emitting `clicked()`; CheckBox, RadioButton and Switch set it. |
| `checked` | `bool` | `false` | Holds the toggle state of a checkable control; setting it from code emits neither `toggled()` nor `clicked()`. |
| `autoExclusive` | `bool` | `false` | Whether a checked control stays checked when activated, so a group never ends up with nothing selected; RadioButton sets it. |
| `highlighted` | `bool` | `false` | Whether the default `contentItem` draws its label bold as a visual emphasis, independent of focus and hover. |
| `hoverEnabled` | `bool` | `false` | Whether the control tracks the pointer and exposes `hovered`; off by default because it enables any-motion mouse reporting for the whole application, and inert while [Terminal](terminal.md)`.mouseReporting` is false. |
| `hovered` | `alias` |  | Read-only. True while `hoverEnabled` is set and the pointer is over the control; always false on terminals without motion reporting. |
| `textColor` | `rgba` | `"default"` | Holds the terminal colour the default `contentItem` uses for its label, where `"default"` is the terminal's own foreground colour. |
| `padding` | `int` | `0` | Holds the default for the four directional paddings, in cells. |
| `topPadding` | `int` | `padding` | Holds the rows between the top edge and `contentItem`. |
| `bottomPadding` | `int` | `padding` | Holds the rows between `contentItem` and the bottom edge. |
| `leftPadding` | `int` | `padding` | Holds the cells between the left edge and `contentItem`. |
| `rightPadding` | `int` | `padding` | Holds the cells between `contentItem` and the right edge. |
| `availableWidth` | `int` |  | Read-only. Holds `width` minus the horizontal paddings, never negative: the width given to `contentItem`. |
| `availableHeight` | `int` |  | Read-only. Holds `height` minus the vertical paddings, never negative: the height given to `contentItem`. |
| `background` | `Item` | `null` | Holds the item drawn behind everything else, resized to fill the control at `z` -1; each control supplies a default, and any Item assigned here replaces it. |
| `contentItem` | `Item` | `null` | Holds the item drawn inside the paddings, resized to `availableWidth` by `availableHeight`; each control supplies a default, and any Item assigned here replaces it. |

## Signals

| Signal | Description |
| :--- | :--- |
| `clicked()` | Emitted when the control is activated with Enter or Space while focused, or by a mouse press and release inside it; for a checkable control it follows `toggled()`. |
| `toggled(bool checked)` | Emitted with the new state when `toggle()` changes `checked`, whether from activation or a direct call; not emitted when `checked` is assigned. |

## Methods

| Method | Description |
| :--- | :--- |
| `var toggle()` | Flips `checked` and emits `toggled()` on a checkable control; does nothing when `checkable` is false, or when `autoExclusive` is set and the control is already checked. |

## Detailed Description

AbstractButton holds everything about a button-like control except how it
looks, the same split as Qt Quick Controls. It is an [Item](item.md) with a
`MouseArea` filling it, a `Keys` handler, and two slots for visuals:
`background`, resized to the whole control and stacked at `z` -1, and
`contentItem`, placed at (`leftPadding`, `topPadding`) and resized to
`availableWidth` by `availableHeight`. When the paddings leave no room, for
example a one-row [Button](button.md) whose default paddings assume a border,
the content is instead centred in the control at its implicit size, so a label
never disappears. The implicit size is the content's implicit size plus the
paddings, or the background's implicit size if that is larger. Whenever
`background` or `contentItem` is assigned, the item becomes a visual child of
the control and is laid out at once; the control's own default item stays but
is hidden.

Activation is one path for keyboard and mouse: Enter (`Qt.Key_Return`) or
Space while the control has active focus, or a mouse press and release inside
it, first calls `toggle()` if `checkable`, then emits `clicked()`. A mouse
press also gives the control focus and holds `pressed` until the release;
`down` mirrors `pressed` unless a style assigns it. Nothing happens while
`enabled` is false, and the engine withholds keys and clicks from disabled
items anyway; the control is `focusable` exactly while `enabled`.

Use this type directly only to build a new control. To restyle an existing
one, assign `background` and `contentItem` on the control itself. The
control's properties are reachable from the assigned items through the
control's `id`, or through `parent` once the item has been adopted:

```qml
Button {
    id: save
    text: "Save"
    background: Rectangle {
        color: save.down ? "#2f6f4f" : "#65e6a0"
        border.style: save.activeFocus ? Border.Double : Border.Single
    }
    contentItem: Text {
        text: save.text
        color: "#0b0f14"
        bold: save.activeFocus
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
    }
}
```

### Example

A minimal toggle built on the template, with its own look for both states.

```qml
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    AbstractButton {
        id: pin
        anchors.centerIn: parent
        checkable: true
        text: pin.checked ? "Pinned" : "Pin"
        padding: 1
        focus: true
        background: Rectangle {
            color: pin.checked ? "#65e6a0" : "default"
            border.style: pin.activeFocus ? Border.Double : Border.Single
        }
        contentItem: Text {
            text: pin.text
            color: pin.checked ? "#0b0f14" : "default"
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
        }
        onToggled: checked => console.log("pinned:", checked)
    }
}
```
