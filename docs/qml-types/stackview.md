---
title: StackView
kind: qml
import: QmlTui.Controls
since: "0.4"
inherits: Item
cpp: qmltui::Item
category: overlays and navigation
summary: A navigation stack that shows only its topmost page and creates pages on demand from URLs, components or items.
---

# StackView QML type

A navigation stack that shows only its topmost page and creates pages on demand from URLs, components or items. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui.Controls` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `initialItem` | `var` | `null` | Holds a QML file URL string, `Component`, or `Item` pushed automatically on completion when the stack is empty. |
| `currentItem` | `Item` | `null` | Read-only. Holds the page on top of the stack, or `null` when the stack is empty. |
| `depth` | `int` | `0` | Read-only. Holds the number of pages on the stack; `push()`, `replace()`, `pop()`, and `clear()` keep it current. |

## Methods

| Method | Description |
| :--- | :--- |
| `Item push(var itemOrUrl, var properties = {})` | Creates or adopts a page from a QML file URL string, a `Component`, or an existing `Item`, applies `properties` when creating from a URL or `Component` (ignored for an existing `Item`), hides the previous top, and returns the new page or `null` on failure. |
| `Item replace(var itemOrUrl, var properties = {})` | Creates or adopts a replacement top page from a QML file URL string, a `Component`, or an existing `Item`, applies `properties` when creating from a URL or `Component` (ignored for an existing `Item`), removes the previous top page (destroying it if the view created it and hiding it otherwise), and returns the new page or `null` on failure. |
| `Item pop()` | Removes the top page, destroying it if the view created it and hiding it otherwise, and shows the one beneath, returning that page; returns `null` without popping when the stack holds one page or none. |
| `void clear()` | Removes every page, destroying the ones the view created and hiding the rest, and sets `currentItem` to `null`. |

## Detailed Description

StackView is written in QML on top of [Item](item.md). It holds its pages in
an inner container that fills the view; when `initialItem` is non-null on
completion and the stack is empty, it is pushed automatically. `push()` and
`replace()` accept either a URL string,
which is loaded with `Qt.createComponent()` and must be ready synchronously,
a `Component` created with `createObject()`, or an already existing `Item`,
which is reparented into the view. The optional `properties` object is passed
to `createObject()` for the first two forms and ignored for an existing item.
Each pushed or replaced page is moved to (0, 0) and resized to the view's
`width` and `height`, and the view keeps the current page at that size when it
is resized. On `push()`, the previous top page is hidden, not destroyed; on
`replace()`, the previous top page is destroyed if the view created it or
hidden otherwise.

`pop()` removes the topmost page and reveals and resizes the page below it.
A page the view created from a URL or a `Component` is destroyed; a page that
was pushed as an existing `Item` is only hidden and stays owned by whoever
created it. The bottom page can
never be popped; call `clear()` to empty the stack entirely. Pages are
switched instantly, with no transition. StackView saves active keyboard focus
per page: when the outgoing page (or the StackView itself) holds active focus,
`push()` records the focused item on that page and moves active focus to the
first visible, enabled focusable item (`focusPolicy !== Qt.NoFocus`) inside the
new page (or the page itself); `replace()` preserves the saved focus entry for
the page below while focusing the replacement page; and `pop()` restores active
focus to the saved item on the revealed page (when still visible and enabled)
or falls back to its first focusable item.

`depth` follows the stack: every `push()`, `replace()`, `pop()`, and `clear()` updates it,
so bindings such as a "Back" button's `visible: stack.depth > 1` work. Use
[StackLayout](stacklayout.md) instead when all pages exist up front and are
chosen by index, for example from a [TabBar](tabbar.md).

### Example

<<< ../../examples/gallery/examples/StackView.qml{qml}

### Gallery Example

![StackView recorded demo](../assets/casts/components/stack_view.gif)

See [`examples/gallery/examples/StackView.qml`](../../examples/gallery/examples/StackView.qml) (`./build/examples/gallery/qmltui_gallery --example StackView`).
