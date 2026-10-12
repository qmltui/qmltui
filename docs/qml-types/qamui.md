---
title: Qamui
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::QamuiGlobal
category: core
summary: Global Qamui engine singleton providing helper factory methods.
---

# Qamui Singleton

Global Qamui engine singleton providing helper factory methods. [More...](#detailed-description)

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
| `Color rgba(real red, real green, real blue, real alpha)` | Constructs an rgba color value from channel components in the 0.0 to 1.0 range. |

## Detailed Description

`Qamui.span(text)` creates a [span](span.md) with inherited style.
`Qamui.span(text, options)` accepts `color`, `backgroundColor`, `bold`, `dim`,
`italic`, `underline`, and `reversed`. Colors accept strings or Qamui `rgba`
values; attributes require booleans. Unknown options and invalid values emit
warnings and are ignored, leaving the corresponding overrides unset.

`Qamui.rgba(red, green, blue, alpha)` constructs a color value directly from
normalized channel floats (0.0 to 1.0).

Use the returned spans in [Text.spans](text.md).

### Example

```qml
import QtQml
import Qamui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Text {
        anchors.centerIn: parent
        spans: [
            Qamui.span("Hello ", { "color": "cyan", "bold": true }),
            Qamui.span("Terminal", { "color": "yellow", "underline": true })
        ]
    }
}
```
