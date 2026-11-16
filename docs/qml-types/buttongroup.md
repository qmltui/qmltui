---
title: ButtonGroup
kind: qml
import: QmlTui.Controls
since: "0.4"
inherits: QtObject
cpp: QObject
category: inputs
summary: A non-visual object that links RadioButtons so that checking one unchecks the others
---

# ButtonGroup QML type

A non-visual object that links [RadioButton](radiobutton.md) items so that checking one unchecks the others. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui.Controls` |
| **Since** | QmlTui 0.4 |
| **Inherits** | `QtObject` |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `buttons` | `list<QtObject>` |  | Holds the items declaratively assigned to the group; each member is added to `registeredButtons` and, if it exposes a `buttonGroup` property, has it set to this group. |
| `registeredButtons` | `var` | `[]` | Holds the array of currently registered buttons, populated both from `buttons` and from controls that set `buttonGroup` directly or call `addButton()`. |

## Methods

| Method | Description |
| :--- | :--- |
| `void addButton(var btn)` | Appends `btn` to `registeredButtons` when non-null and not already present. |
| `void removeButton(var btn)` | Removes `btn` from `registeredButtons`. |
| `void uncheckExcept(var activeButton)` | Sets `checked` to `false` on every other registered button that exposes `checked` and has either no `buttonGroup` property or `buttonGroup` equal to this group. |

## Detailed Description

ButtonGroup is a plain `QtObject`, not an item: it has no geometry, is not
drawn and cannot be anchored or placed in a layout. Declare it anywhere in
the object tree and either assign member items to `buttons` or bind each
[RadioButton](radiobutton.md)'s `buttonGroup` property directly to the group.
Whenever `buttons` changes, the group registers each item via `addButton()` and
assigns itself to the `buttonGroup` property of every member that exposes one.
Conversely, setting `RadioButton.buttonGroup` automatically registers the
button via `addButton()` on assignment or completion and calls `removeButton()`
when reassigned or destroyed. Members without a `buttonGroup` property, such as
[Button](button.md) or [CheckBox](checkbox.md), are still registered in
`registeredButtons` when listed in `buttons`.

When a [RadioButton](radiobutton.md) becomes checked, it calls
`buttonGroup.uncheckExcept(control)` so every other registered member in the
group has `checked` set to `false`. ButtonGroup has no `checkedButton` property
and no signals. Removing an item from the `buttons` list property does not
automatically unregister it or reset its `buttonGroup` to `null`; call
`removeButton(btn)` or clear `btn.buttonGroup` instead.

### Example

<<< ../../examples/gallery/examples/RadioButton.qml{qml}

### Gallery Example

![RadioButton recorded demo](../assets/casts/components/radio_button.gif)

See [`examples/gallery/examples/RadioButton.qml`](../../examples/gallery/examples/RadioButton.qml) (`./build/examples/gallery/qmltui_gallery --example RadioButton`).
