---
title: Behavior
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::Behavior
category: animation and state
summary: Defines default animations for property changes.
---

# Behavior QML type

Defines default animations for property changes. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `animation` | `Animation` | `null` | Holds the animation to execute whenever the target property changes value. This is the default property. |

## Detailed Description

Behavior allows an animation to be triggered automatically whenever a property's
value changes, using the `Behavior on <property>` syntax.

Whenever the target property is assigned a new value, the Behavior intercepts
the change and plays its child [Animation](animation.md) to transition the property
smoothly from its old value to the new value.

> [!NOTE]
> Behavior is an optional feature in Qamui requiring `QAMUI_ENABLE_BEHAVIOR=ON`
> and links against `Qt6::QmlPrivate`.

### Example

```qml
Rectangle {
    id: rect
    width: 20
    height: 5

    Behavior on width {
        NumberAnimation {
            duration: 300
            easing: Easing.OutQuad
        }
    }
}
```
