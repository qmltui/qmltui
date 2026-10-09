---
title: span
kind: qml
import: Qtui
since: "0.4"
inherits: 
cpp: qtui::Span
category: core
summary: A literal text run with optional style overrides.
---

# span Value type

A literal text run with optional style overrides. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | none |
| **Instantiation** | Object literal or `Qtui.span()` |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` |  | _TODO_ |
| `color` | `rgba` |  | _TODO_ |
| `backgroundColor` | `rgba` |  | _TODO_ |
| `bold` | `bool` |  | _TODO_ |
| `dim` | `bool` |  | _TODO_ |
| `italic` | `bool` |  | _TODO_ |
| `underline` | `bool` |  | _TODO_ |
| `reversed` | `bool` |  | _TODO_ |

## Detailed Description

Use spans through [Text.spans](text.md). Create them with JavaScript object
literals or the [Qtui.span()](qtui.md) factory. Both accept color strings and
Qtui `rgba` values. Attribute omission is distinct from explicit `false`.

In C++, `hasBold()` (and the corresponding methods for other attributes)
reports whether an override exists. A boolean getter alone does not report
presence. `resetBold()`, `resetDim()`, `resetItalic()`, `resetUnderline()`, and
`resetReversed()` restore inheritance; color reset methods restore unset colors.
The QML properties support reset by assigning `undefined`.

Equality includes override presence, so an unset attribute differs from an
explicitly disabled attribute. Copies are ordinary values with no QObject
identity. See Text for multiline, grapheme-boundary, and gradient semantics.
