---
title: ColorAnimation
kind: qml
import: QmlTui
since: "0.4"
inherits: PropertyAnimation
cpp: qmltui::ColorAnimation
category: animation and state
summary: Animates color properties over time by smoothly interpolating color channels.
---

# ColorAnimation QML type

Animates color properties over time by smoothly interpolating color channels. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [PropertyAnimation](propertyanimation.md) |

## Detailed Description

ColorAnimation is a specialized [PropertyAnimation](propertyanimation.md) that
smoothly transitions between color values by interpolating their RGBA channels.
It is commonly used on properties like `color`, `textColor`, and `border.color`.

Like all PropertyAnimations, it supports `duration`, `from`, `to`, and `easing`.

### Example

<<< ../../examples/gallery/examples/Animation.qml{qml}

### Gallery Example

![Animation recorded demo](../assets/casts/components/animation.gif)

See [`examples/gallery/examples/Animation.qml`](../../examples/gallery/examples/Animation.qml) (`./build/examples/gallery/qmltui_gallery --example Animation`, or open in the [WebAssembly Playground](../playground.md?example=Animation.qml)).
