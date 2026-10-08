---
title: StackView
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Item
cpp: qtui::Item
category: overlays and navigation
summary: A navigation stack that shows only its topmost page and creates pages on demand from URLs, components or items.
---

# StackView QML type

A navigation stack that shows only its topmost page and creates pages on demand from URLs, components or items. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `currentItem` | `alias` |  | Read-only. Holds the page on top of the stack, or `null` when the stack is empty. |
| `depth` | `int` |  | Read-only. Holds the number of pages on the stack as reported by the `stackItems` array. |
| `stackItems` | `var` | `[]` | Holds the JavaScript array of pages from bottom to top; it is managed by `push()`, `pop()` and `clear()`. |

## Methods

| Method | Description |
| :--- | :--- |
| `var push(var itemOrUrl, var properties)` | Creates or adopts a page from a QML file URL string, a `Component` or an existing `Item`, applies `properties`, hides the previous top, and returns the new page or `null` on failure. |
| `var pop()` | Destroys the top page and shows the one beneath it, returning that page; returns `null` without popping when the stack holds one page or none. |
| `var clear()` | Destroys every page and sets `currentItem` to `null`. |

## Detailed Description

StackView is written in QML on top of [Item](item.md). It holds its pages in
an inner container that fills the view; `push()` accepts either a URL string,
which is loaded with `Qt.createComponent()` and must be ready synchronously,
a `Component` created with `createObject()`, or an already existing `Item`,
which is reparented into the view. The optional `properties` object is passed
to `createObject()` for the first two forms and ignored for an existing item.
Each pushed page is moved to (0, 0) and resized to the view's `width` and
`height`, and the view keeps the current page at that size when it is
resized. The previous top page is hidden, not destroyed.

`pop()` destroys the topmost page, including a page that was pushed as an
existing item, and reveals and resizes the page below it. The bottom page can
never be popped; call `clear()` to empty the stack entirely. Pages are
switched instantly, with no transition, and StackView does not move keyboard
focus; set `focus` on a control inside the new page if it should receive
input.

`depth` is bound to `stackItems.length`, but the methods modify the array in
place, so the binding is not re-evaluated and `depth` does not update after
the initial evaluation. Use [StackLayout](stacklayout.md) instead when all
pages exist up front and are chosen by index, for example from a
[TabBar](tabbar.md).

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    StackView {
        id: pages
        anchors.fill: parent
        Component.onCompleted: push(homePage)
    }

    Component {
        id: homePage
        Column {
            spacing: 1
            Button { text: "Open details"; focus: true
                     onClicked: pages.push(detailPage, { label: "Details" }) }
            Button { text: "Quit"; onClicked: Qt.quit() }
        }
    }

    Component {
        id: detailPage
        Panel {
            property string label: ""
            title: label
            Button { x: 2; y: 1; text: "Back"; focus: true; onClicked: pages.pop() }
        }
    }
}
```
