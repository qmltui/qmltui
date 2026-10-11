---
title: Qamui
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::QamuiGlobal
category: core
summary: _TODO_
---

# Qamui Singleton

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |
| **Instantiation** | Singleton, use by name |

## Methods

| Method | Description |
| :--- | :--- |
| `span span(string text)` | Creates a literal span with inherited style. |
| `span span(string text, QVariantMap options)` | Creates a span with explicit style overrides. |
| `Color rgba(real red, real green, real blue, real alpha)` | _TODO_ |

## Detailed Description

`Qamui.span(text)` creates a [span](span.md) with inherited style.
`Qamui.span(text, options)` accepts `color`, `backgroundColor`, `bold`, `dim`,
`italic`, `underline`, and `reversed`. Colors accept strings or Qamui `rgba`
values; attributes require booleans. Unknown options and invalid values emit
warnings and are ignored, leaving the corresponding overrides unset.

Use the returned values in [Text.spans](text.md).

### Example

```qml
import QtQml
import Qamui

Terminal {
    // _TODO_ show Qamui in use
}
```
