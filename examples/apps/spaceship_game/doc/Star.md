# Star

## 1. Component Overview

The `Star` component represents a starfield background particle in the Spaceship Game arcade example. It creates a parallax scrolling star effect by varying particle representation (`.`, `+`, `*`) and brightness based on a scroll `speed` property.

Developers use `Star` for multi-layered parallax terminal background effects.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
- **File Location**: `examples/spaceship_game/Star.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Text`
  - 1x1 text element displaying background star characters.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `speed` | `int` | `1` | No | Parallax layer depth/speed (`1`: slow `.`, `2`: medium `+`, `3`: fast `*`). |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Parallax Styling**: Binds character glyph and color dimming to `speed`.

## 8. Usage Example

```qml
import QtQml
import Qtui

Star {
    speed: 2
    x: 30
    y: 12
}
```
