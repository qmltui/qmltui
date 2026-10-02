# NavTabBar

## 1. Component Overview

The `NavTabBar` component displays the navigation bar at the top or bottom of the PLC HMI screen. It splits the available width into four tab items corresponding to function keys: `[F1] DASHBOARD`, `[F2] ALARMS`, `[F3] DIAGNOSTICS`, and `[F4] SETTINGS`. It visually highlights the currently selected tab and alerts the operator when unhandled alarms are present.

Developers use `NavTabBar` to implement function-key tabbed view navigation.

## 2. Project Structure and Dependencies

- **Import Dependencies**:
  - `QtQml` (version 2.15)
  - `Tui` (version 1.0)
  - `IndustrialPlcHmi` (version 1.0)
- **File Location**: `examples/industrial_plc_hmi/NavTabBar.qml`

## 3. Component Hierarchy and Role

- **Base Type**: `Rectangle`
  - Horizontal bar (`height: 1`) containing a `Row` of 4 tab `Rectangle` delegates.

## 4. Properties

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|
| `currentTab` | `int` | `0` | No | Index of the currently active tab (`0`: Dashboard, `1`: Alarms, `2`: Diagnostics, `3`: Settings). |
| `alarmCount` | `int` | `0` | No | Number of active alarms. When greater than zero, the Alarms tab is highlighted in red. |

## 5. Signals

*This component declares no custom signals.*

## 6. Methods

*This component declares no custom JavaScript functions.*

## 7. Inter-Component Interactions

- **Theme Integration**: Uses `HmiTheme.borderActive`, `HmiTheme.textSecondary`, and `HmiTheme.textDanger` for active tab selection and alarm notifications.

## 8. Usage Example

```qml
import QtQml 2.15
import Qtui
import IndustrialPlcHmi 1.0

NavTabBar {
    width: parent.width
    currentTab: 1
    alarmCount: 3
}
```
