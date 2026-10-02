# Explosion

## 1. Component Overview

The `Explosion` component represents animated explosion particles when ships or projectiles are destroyed in the Spaceship Game arcade example. It cycles ASCII characters (`*`, `#`, `+`, `.`) and colors (`orange`, `red`, `yellow`, `gray`) based on an animation `frame` property.

Developers use `Explosion` to render temporary impact effects.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
- **File Location**: `examples/spaceship_game/Explosion.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Text`
  - 1x1 text element displaying animated particle characters.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `frame` | `int` | `0` | No | Explosion animation frame index (`0` to `3`). |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Frame Animation**: Binds `text` and `color` to the `frame` property to simulate fading particle decay.

## 8. Usage Example

```qml
import QtQml 2.15
import Terminal 1.0

Explosion {
    frame: 1
    x: 12
    y: 8
}
```
