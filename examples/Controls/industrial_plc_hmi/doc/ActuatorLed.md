# ActuatorLed

## 1. Component Overview

The `ActuatorLed` component represents an industrial actuator or valve status indicator in the PLC HMI interface. It displays the state of an actuator (e.g. `[VALVE: ON]` vs `[VALVE: OFF]`) with active/inactive color coding and bold text styling.

Developers use `ActuatorLed` in control panels and status views to give operators clear visual feedback on valve, pump, or motor states.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
  - `IndustrialPlcHmi` (version 1.0)
- **File Location**: `examples/industrial_plc_hmi/ActuatorLed.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Rectangle`
  - Acts as a background container dynamically colored based on activation state.
  - Composes an internal `Text` child centered within the rectangle.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `name` | `string` | `"VALVE"` | No | Identifier label for the actuator (e.g., `"VALVE A"`, `"PUMP 1"`). |
| `active` | `bool` | `false` | No | Indicates whether the actuator is active (ON) or inactive (OFF). |
| `activeColor` | `string` | `"#008700"` | No | Background color when `active` is true. |
| `inactiveColor` | `string` | `"#303030"` | No | Background color when `active` is false. |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Theme Binding**: Reads `HmiTheme.textPrimary` and `HmiTheme.textDim` to style text according to the `active` state.
- **Parent Sizing**: Computes default width relative to parent container (`Math.floor((parent.width - 1) / 2)`).

## 8. Usage Example

```qml
import QtQml 2.15
import Qtui
import IndustrialPlcHmi 1.0

ActuatorLed {
    name: "VALVE A"
    active: true
    activeColor: "#008700"
}
```
