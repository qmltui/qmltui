---
title: QmlTui
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::QmlTuiGlobal
category: core
summary: Global QmlTui engine singleton providing helper factory methods.
---

# QmlTui Singleton

Global QmlTui engine singleton providing helper factory methods. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |
| **Instantiation** | Singleton, use by name |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `defaultColor` | `rgba` | `"default"` | Read-only. Holds the terminal's default foreground/background color (`Color::Default`). |
| `transparent` | `rgba` | `"transparent"` | Read-only. Holds the transparent color sentinel (`Color::Transparent`). |

## Methods

| Method | Description |
| :--- | :--- |
| `span span(string text)` | Creates a literal span with inherited style. |
| `span span(string text, QVariantMap options)` | Creates a span with explicit style overrides. |
| `Color rgba(real red, real green, real blue, real alpha)` | Constructs an rgba color value from channel components in the 0.0 to 1.0 range. |
| `Color ansi(int index)` | Constructs an rgba color value for ANSI palette index `0` through `15`. |

## Detailed Description

`QmlTui.span(text)` creates a [span](span.md) with inherited style.
`QmlTui.span(text, options)` accepts `color`, `backgroundColor`, `bold`, `dim`,
`italic`, `underline`, and `reversed`. Colors accept strings or QmlTui `rgba`
values; attributes require booleans. Unknown options and invalid values emit
warnings and are ignored, leaving the corresponding overrides unset.

`QmlTui.rgba(red, green, blue, alpha)` constructs a color value directly from
normalized channel floats (0.0 to 1.0). `QmlTui.ansi(index)` constructs an ANSI
palette color (`0` to `15`), and `QmlTui.defaultColor` / `QmlTui.transparent`
provide pre-constructed `rgba` constants without string parsing.

Use the returned spans in [Text.spans](text.md).

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

    Text {
        anchors.centerIn: parent
        spans: [
            QmlTui.span("Hello ", { "color": "cyan", "bold": true }),
            QmlTui.span("Terminal", { "color": "yellow", "underline": true })
        ]
    }
}
```
