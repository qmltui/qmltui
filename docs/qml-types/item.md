---
title: Item
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::Item
category: core
summary: Base visual type providing cell geometry, layout anchors, visibility, focus, and state management.
---

# Item QML type

Base visual type providing cell geometry, layout anchors, visibility, focus, and state management. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `x` | `int` | `0` | Holds the horizontal position in character cells relative to its parent item. |
| `y` | `int` | `0` | Holds the vertical position in rows relative to its parent item. |
| `z` | `real` | `0.0` | Holds the stacking order relative to siblings; higher values render on top. |
| `width` | `int` | `0` | Holds the explicit width in character cells. |
| `height` | `int` | `0` | Holds the explicit height in rows. |
| `implicitWidth` | `int` | `0` | Holds the natural, unconstrained width based on item content. |
| `implicitHeight` | `int` | `0` | Holds the natural, unconstrained height based on item content. |
| `visible` | `bool` | `true` | Whether the item is rendered and participates in layouts. |
| `enabled` | `bool` | `true` | Whether the item receives user input events. |
| `focus` | `bool` | `false` | Requests keyboard focus within its focus scope. |
| `activeFocus` | `bool` |  | Read-only. Whether the item currently has active keyboard input focus. |
| `focusPolicy` | `enumeration` | `Qt.NoFocus` | The way the item accepts focus (`Qt.NoFocus`, `Qt.TabFocus`, `Qt.ClickFocus`, `Qt.StrongFocus`, `Qt.WheelFocus`). |
| `parent` | `Item` | `null` | Holds the visual parent item in the scene graph hierarchy. |
| `anchors` | `Anchors` |  | Read-only. Holds the anchor configuration ([Anchors](anchors.md)) for relative positioning. |
| `left` | `AnchorLine` |  | Read-only. Holds the left edge anchor line. |
| `right` | `AnchorLine` |  | Read-only. Holds the right edge anchor line. |
| `top` | `AnchorLine` |  | Read-only. Holds the top edge anchor line. |
| `bottom` | `AnchorLine` |  | Read-only. Holds the bottom edge anchor line. |
| `horizontalCenter` | `AnchorLine` |  | Read-only. Holds the horizontal center anchor line. |
| `verticalCenter` | `AnchorLine` |  | Read-only. Holds the vertical center anchor line. |
| `clip` | `bool` | `false` | Whether rendering of child items is clipped to the item's rectangular bounds. |
| `padding` | `int` | `0` | Holds the default internal padding in cells applied to all four edges. |
| `leftPadding` | `int` | `padding` | Holds the internal padding in cells along the left edge. |
| `rightPadding` | `int` | `padding` | Holds the internal padding in cells along the right edge. |
| `topPadding` | `int` | `padding` | Holds the internal padding in rows along the top edge. |
| `bottomPadding` | `int` | `padding` | Holds the internal padding in rows along the bottom edge. |
| `data` | `list<QtObject>` |  | Read-only. Default property containing all child items and attached objects. |
| `children` | `list<Item>` |  | Read-only. Holds the list of visual child items. |
| `states` | `list<State>` |  | Read-only. Holds the list of visual [State](state.md) definitions. |
| `transitions` | `list<Transition>` |  | Read-only. Holds the list of [Transition](transition.md) animations between states. |
| `state` | `string` | `""` | Holds the name of the currently active visual state. |

## Signals

| Signal | Description |
| :--- | :--- |
| `dirty()` | Emitted when visual content or layout changes require a screen repaint. |
| `ancestryChanged()` | The item or an ancestor was attached to or detached from a parent, so the item's root [Terminal](terminal.md) may have changed. Unlike `parentChanged`, an ancestor's reparenting — a `Repeater` or `Loader` inserting finished content — reaches every item of the moved subtree. Emitted on each item; connect on the item whose Terminal-derived state needs refreshing. |

## Detailed Description

Item is the base type for all visual components in QmlTui, analogous to `QQuickItem`
in Qt Quick, but operating entirely on discrete integer character cell columns and rows.

It defines spatial position (`x`, `y`), explicit and implicit size (`width`, `height`,
`implicitWidth`, `implicitHeight`), hierarchical composition (`parent`, `children`, `data`),
input focus state (`focus`, `activeFocus`, `focusable`), [Anchors](anchors.md),
clipping, padding, and declarative [State](state.md) machines.

Visual items inherit from Item and implement `paint()` to write characters, colors,
and attributes to the terminal text grid buffer.

### Example

```qml
import QtQml
import QmlTui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Item {
        anchors.centerIn: parent
        width: 24
        height: 6

        Rectangle {
            anchors.fill: parent
            border.style: Border.Single
            color: "#1e1e2e"
        }

        Text {
            anchors.centerIn: parent
            text: "Base Item Container"
            color: "cyan"
        }
    }
}
```
