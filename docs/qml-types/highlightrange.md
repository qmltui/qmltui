---
title: highlightRange
kind: qml
import: QmlTui
since: "0.5"
inherits:
cpp: qmltui::HighlightRange
category: core
summary: A character range with formatting styles for TextArea highlighting.
---

# highlightRange Value type

A character range with formatting styles for TextArea highlighting. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.5 |
| **Inherits** | none |
| **Instantiation** | Object literal |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `start` | `int` | `0` | Holds the zero-based UTF-16 character offset in document text. |
| `length` | `int` | `0` | Holds the length of the highlighted range in UTF-16 code units. |
| `color` | `rgba` |  | Holds the foreground text color override for this range. |
| `backgroundColor` | `rgba` |  | Holds the background cell color override for this range. |
| `bold` | `bool` |  | Whether this range applies bold text formatting. |
| `dim` | `bool` |  | Whether this range applies dim / faint text formatting. |
| `italic` | `bool` |  | Whether this range applies italic text formatting. |
| `underline` | `bool` |  | Whether this range applies underline text formatting. |
| `reversed` | `bool` |  | Whether this range swaps foreground and background colors. |

## Detailed Description

Use `highlightRange` values with [TextArea.setHighlightLayer](textarea.md). Construct them using
JavaScript object literals matching the structured value type:

```qml
textArea.setHighlightLayer("syntax", [
    { start: 0, length: 5, color: "green", bold: true },
    { start: 10, length: 4, color: "cyan" }
], textArea.textRevision)
```

Like `span`, attribute omission is distinct from explicit `false`. An unset attribute allows underlying
layers or base item styling to show through, while an explicit `false` turns off that attribute.
Properties support reset in QML by assigning `undefined`.

In C++, `HighlightRange` shares underlying styling storage with `TextStyle`. Nonempty ranges automatically
expand outward to grapheme cluster boundaries so combining characters and multi-code-unit emoji are never
partially styled.
