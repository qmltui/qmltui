# SettingsView

## 1. Component Overview

The `SettingsView` component allows operators to modify system parameters and safety limits (such as motor target RPM and over-temperature safety cutoffs) using interactive `TextInput` controls.

Developers use `SettingsView` for setpoint configuration screens.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
  - `IndustrialPlcHmi` (version 1.0)
- **File Location**: `examples/industrial_plc_hmi/SettingsView.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Rectangle`
  - Split-view container containing setpoint text inputs on the left and safety limit ceilings on the right.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `targetSpeed` | `int` | `1500` | No | Current target motor RPM setpoint. |
| `maxTempLimit` | `double` | `85.0` | No | Current over-temperature safety trip limit in °C. |

## 5. Signals

#### `speedChanged(int newSpeed)`

- **Parameters**: `newSpeed` (`int`) — updated target motor speed in RPM.
- **Trigger**: Emitted when the user enters a valid integer RPM value (between 0 and 3000) into the target speed text input and presses Enter.

#### `tempLimitChanged(double newLimit)`

- **Parameters**: `newLimit` (`double`) — updated temperature cutoff limit in °C.
- **Trigger**: Emitted when the user enters a valid temperature limit (between 30.0 and 110.0 °C) into the temperature input and presses Enter.

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Focus Styling**: Updates input border colors dynamically using `setRpmInput.activeFocus` and `setTempInput.activeFocus`.

## 8. Usage Example

```qml
import QtQml
import Qamui
import IndustrialPlcHmi 1.0

SettingsView {
    targetSpeed: 1800
    maxTempLimit: 90.0
    onSpeedChanged: (newSpeed) => { console.log("New Speed:", newSpeed) }
    onTempLimitChanged: (newLimit) => { console.log("New Temp Limit:", newLimit) }
}
```
