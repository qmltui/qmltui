# Bullet

## 1. Component Overview

The `Bullet` component represents a projectile fired by the player's ship in the Spaceship Game terminal arcade example. It renders as a yellow `^` character.

Developers use `Bullet` as an entity delegate inside game loops or repeater models.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
- **File Location**: `examples/spaceship_game/Bullet.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Text`
  - Fixed size 1x1 character element displaying text `"^"` with yellow foreground color.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `text` | `string` | `"^"` | No | Fixed character representation of the projectile. |
| `color` | `string` | `"yellow"` | No | Foreground color of the projectile. |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Parent Layout**: Positioned dynamically by `x` and `y` coordinates driven by the main game loop.

## 8. Usage Example

```qml
import QtQml 2.15
import Qtui

Bullet {
    x: 10
    y: 15
}
```
