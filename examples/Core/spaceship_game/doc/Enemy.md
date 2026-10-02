# Enemy

## 1. Component Overview

The `Enemy` component represents hostile target ships in the Spaceship Game terminal arcade example. It dynamically selects text ASCII art (`v`, `<M>`, `oXo`) and element width based on the enemy `type` property.

Developers use `Enemy` to render varying enemy ship types in game models.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
- **File Location**: `examples/spaceship_game/Enemy.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Text`
  - Dynamically sized 1x1 or 3x1 text element.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `type` | `int` | `0` | No | Enemy class type (`0`: Scout `v`, `1`: Medium `<M>`, `2`: Boss `oXo`). |
| `enemyColor` | `string` | `"red"` | No | Foreground color string for the enemy sprite. |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Dynamic Formatting**: Binds its `text` property to evaluate `type` and selects sprite graphics dynamically.

## 8. Usage Example

```qml
import QtQml 2.15
import Terminal 1.0

Enemy {
    type: 1
    enemyColor: "magenta"
    x: 20
    y: 5
}
```
