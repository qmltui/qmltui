# PowerUp

## 1. Component Overview

The `PowerUp` component represents collectible bonus items (`[S]` Shield, `[F]` Firepower, `[H]` Health) dropped by enemies in the Spaceship Game arcade example.

Developers use `PowerUp` to render falling power-up item delegates.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
- **File Location**: `examples/spaceship_game/PowerUp.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Text`
  - 3x1 text element displaying bracketed item symbols.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `type` | `string` | `"shield"` | No | Power-up type (`"shield"`, `"fire"`, `"health"`). |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Type Formatting**: Selects bracket symbol (`[S]`, `[F]`, `[H]`) and color based on `type`.

## 8. Usage Example

```qml
import QtQml 2.15
import Qtui

PowerUp {
    type: "fire"
    x: 15
    y: 10
}
```
