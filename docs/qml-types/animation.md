---
title: Animation
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::Animation
category: animation and state
summary: Base class for all QmlTui animations.
---

# Animation QML type

Base class for all QmlTui animations. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not directly instantiable |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `running` | `bool` | `false` | Holds whether the animation is currently running. |
| `loops` | `int` | `1` | Holds the number of times the animation plays before completing. Set to `Animation.Infinite` (or any negative integer) to loop indefinitely without emitting `finished()`. |

## Enumeration Animation.LoopCount

| Value | Description |
| :--- | :--- |
| `Animation.Infinite` | Repeat the animation indefinitely (`-1`). |

## Signals

| Signal | Description |
| :--- | :--- |
| `started()` | Emitted when the animation begins playing. |
| `stopped()` | Emitted when the animation is stopped before completing. |
| `finished()` | Emitted when the animation completes all specified loops. |

## Methods

| Method | Description |
| :--- | :--- |
| `void start()` | Starts or resumes playback of the animation. |
| `void stop()` | Stops the animation and resets playback. |

## Detailed Description

Animation is the uncreatable abstract base class for all animation types in QmlTui,
including [PropertyAnimation](propertyanimation.md), [NumberAnimation](numberanimation.md),
and [ColorAnimation](coloranimation.md).

It provides common playback controls (`start()`, `stop()`), loop management (`loops`),
and lifecycle signals (`started`, `stopped`, `finished`). Under the hood, running
animations are stepped by the QmlTui animation manager driven by the active
system or virtual clock.

### Example

<<< ../../examples/gallery/examples/Animation.qml{qml}

### Gallery Example

![Animation recorded demo](../assets/casts/components/animation.gif)

See [`examples/gallery/examples/Animation.qml`](../../examples/gallery/examples/Animation.qml) (`./build/examples/gallery/qmltui_gallery --example Animation`).
