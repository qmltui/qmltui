# Qtui Framework Technical Guide

Qtui is a modern retained-mode terminal UI framework designed for Qt 6.

## Retained-Mode Terminal Architecture

Unlike traditional immediate-mode TUI libraries, Qtui uses a scene graph and retained-mode terminal items:

1. **Items & Hierarchies**: Subclass `qtui::Item` to provide custom rendering into `TextGridBuffer`.
2. **Layout Engine**: Automatic declarative positioning with `Row`, `Column`, `GridLayout`, and `Anchors`.
3. **QML Declarative Bindings**: Full reactive properties and signal-slot mechanisms.
4. **Behavior on Properties**: Smooth animated value transitions in terminals.
5. **Testing Framework**: Native `TestRunner` closed-loop automation engine.

## Core QML Modules

- `Qtui`: Core terminal primitives (`Terminal`, `Item`, `Rectangle`, `Text`, `Paragraph`, `ListView`, `ScrollView`).
- `Qtui.Controls`: Pre-built interactive widgets (`Button`, `CheckBox`, `RadioButton`, `Slider`, `Switch`, `Dialog`, `Popup`, `Calendar`).
- `Qtui.Extras`: Visual indicators (`BigText`, `Gauge`, `Sparkline`, `TabHeader`, `ToastManager`).
- `Qtui.Image`: Textual pixel art and image rendering (`Image`, `ImageGrid`).
- `Qtui.TerminalHost`: Embedded Unix PTY virtual terminal host.

## Closed-Loop Agent Workflow

Inspect initial frames or run scripted scenarios without launching full terminal emulators:

```bash
QTUI_INSPECT=1 ./build/examples/apps/markdown_viewer/qtui_markdown_viewer
```
